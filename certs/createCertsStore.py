#!/usr/bin/env python3

# Script to download certificates from multiple hosts
# and generate certsstore (ar archive with DER certificates).
# FIXED VERSION: Proper handling of ar archive format to prevent corruption of last certificate
# FILTERED VERSION: Only CA certificates (including root and intermediate) are included.
# released to public domain

import urllib.request
import re
import ssl
import sys
import socket
import argparse
import datetime
import hashlib
import time
import os
import shutil
import struct

from cryptography import x509
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.serialization import Encoding
from cryptography.x509.oid import ExtensionOID

def is_ca_certificate(cert):
    """
    Check if the certificate is a CA certificate.
    Returns True if the certificate has BasicConstraints extension with CA=True.
    """
    try:
        basic_constraints = cert.extensions.get_extension_for_oid(ExtensionOID.BASIC_CONSTRAINTS)
        return basic_constraints.value.ca
    except x509.extensions.ExtensionNotFound:
        # No BasicConstraints extension -> not a CA certificate
        return False
    except Exception:
        # Any other error -> assume not CA
        return False

def extract_certificates(data, seen_urls=None):
    """Extract all certificates from binary data, including CA issuers."""
    if seen_urls is None:
        seen_urls = set()

    certificates = []

    try:
        xcert = x509.load_der_x509_certificate(data)
    except Exception:
        try:
            xcert = x509.load_pem_x509_certificate(data)
        except Exception:
            # Try PKCS7 format
            try:
                from cryptography.hazmat.primitives.serialization import pkcs7
                pkcs7_certs = pkcs7.load_der_pkcs7_certificates(data)
                if pkcs7_certs:
                    xcert = pkcs7_certs[0]
                else:
                    return certificates
            except Exception:
                try:
                    from cryptography.hazmat.primitives.serialization import pkcs7
                    pkcs7_certs = pkcs7.load_pem_pkcs7_certificates(data)
                    if pkcs7_certs:
                        xcert = pkcs7_certs[0]
                    else:
                        return certificates
                except Exception:
                    return certificates

    certificates.append(xcert)

    # Extract CA issuers URLs from Authority Information Access extension
    ca_urls = []
    try:
        for ext in xcert.extensions:
            if ext.oid == x509.ObjectIdentifier("1.3.6.1.5.5.7.1.1"):
                for desc in ext.value:
                    if desc.access_method == x509.oid.AuthorityInformationAccessOID.CA_ISSUERS:
                        ca_urls.append(desc.access_location.value)
    except Exception:
        pass

    # Download CA certificates recursively
    for ca_url in ca_urls:
        if ca_url in seen_urls:
            continue
        seen_urls.add(ca_url)

        try:
            print(f"  Downloading CA certificate from: {ca_url}", file=sys.stderr)
            with urllib.request.urlopen(ca_url) as response:
                ca_data = response.read()
                ca_certs = extract_certificates(ca_data, seen_urls)
                certificates.extend(ca_certs)
        except Exception as e:
            print(f"  Warning: Failed to download CA certificate from {ca_url}: {e}", file=sys.stderr)

    return certificates

def get_certificates_from_host(hostname, port=443):
    """Get certificates from a specific host."""
    context = ssl.create_default_context()
    context.check_hostname = False
    context.verify_mode = ssl.CERT_NONE

    certificates = []

    try:
        with socket.create_connection((hostname, port)) as sock:
            with context.wrap_socket(sock, server_hostname=hostname) as ssock:
                # Get peer certificate
                cert_bin = ssock.getpeercert(binary_form=True)
                if cert_bin:
                    certs = extract_certificates(cert_bin)
                    certificates.extend(certs)

                # Try to get certificate chain if available
                try:
                    if hasattr(ssock, 'getpeercertchain'):
                        chain = ssock.getpeercertchain()
                        for chain_cert in chain:
                            chain_certs = extract_certificates(chain_cert)
                            certificates.extend(chain_certs)
                except Exception:
                    pass
    except Exception as e:
        print(f"Error connecting to {hostname}:{port}: {e}", file=sys.stderr)

    return certificates

def verify_der_file(file_path):
    """Verify that file contains a valid DER X.509 certificate."""
    try:
        with open(file_path, 'rb') as f:
            der_data = f.read()
        x509.load_der_x509_certificate(der_data)
        return True
    except Exception as e:
        print(f"    DER verification failed for {file_path}: {e}", file=sys.stderr)
        return False

def create_bsd_ar_archive_manual(certificates, output_path, keep_der=False):
    """
    Create BSD ar archive manually with proper padding to avoid corruption.
    BSD ar format:
    - Magic: "!<arch>\n" (8 bytes)
    - For each member:
      - Header: 60 bytes (name, timestamp, uid, gid, mode, size, magic)
      - Data: file content
      - Padding: '\n' if size is odd (to align to 2-byte boundary)
    """
    if not certificates:
        print("No certificates to archive", file=sys.stderr)
        return False

    # Create directory for DER files
    if keep_der:
        der_dir = "der_certs"
        os.makedirs(der_dir, exist_ok=True)
        print(f"DER files will be saved in: {os.path.abspath(der_dir)}", file=sys.stderr)
    else:
        import tempfile
        der_dir = tempfile.mkdtemp(prefix="cert_archive_")

    current_time = int(time.time())
    added_count = 0
    valid_certs_data = []  # List of (name, der_data) tuples

    try:
        # Generate and verify all certificates
        for idx, cert in enumerate(certificates, start=1):
            member_name = f"ca_{idx:03d}.der"
            file_path = os.path.join(der_dir, member_name)

            try:
                # Generate DER data
                der_data = cert.public_bytes(Encoding.DER)

                # Write to file for verification
                with open(file_path, 'wb') as f:
                    f.write(der_data)

                # Set modification time explicitly
                os.utime(file_path, (current_time, current_time))

                # Verify the DER file immediately
                if not verify_der_file(file_path):
                    print(f"  ❌ {member_name}: FAILED verification, skipping", file=sys.stderr)
                    continue

                valid_certs_data.append((member_name, der_data))
                added_count += 1

                # Print certificate info
                cn = ""
                try:
                    for dn in cert.subject.rfc4514_string().split(','):
                        keyval = dn.split('=')
                        if keyval[0] == 'CN':
                            cn = keyval[1]
                except Exception:
                    cn = f"cert_{idx}"

                ca_status = "CA" if is_ca_certificate(cert) else "EE"
                print(f"  ✅ {member_name}: CN={cn}, Type={ca_status}, Size={len(der_data)} bytes, Valid: {cert.not_valid_before} to {cert.not_valid_after}",
                      file=sys.stderr)

            except Exception as e:
                print(f"  ❌ Failed to process certificate {idx}: {e}", file=sys.stderr)
                continue

        if added_count == 0:
            print("No valid certificates to archive", file=sys.stderr)
            return False

        # Create BSD ar archive manually
        print(f"\nCreating BSD ar archive with {added_count} certificates...", file=sys.stderr)
        
        with open(output_path, 'wb') as ar_file:
            # Write ar magic
            ar_file.write(b"!<arch>\n")
            
            for member_name, der_data in valid_certs_data:
                data_size = len(der_data)
                
                # BSD ar header format (60 bytes total):
                # File name: 16 bytes (space-padded)
                # Timestamp: 12 bytes (space-padded decimal)
                # UID: 6 bytes (space-padded decimal)
                # GID: 6 bytes (space-padded decimal) 
                # Mode: 8 bytes (space-padded octal)
                # Size: 10 bytes (space-padded decimal)
                # Magic: 2 bytes ("`\n")
                
                # Ensure name fits in 16 bytes
                if len(member_name) > 16:
                    member_name = member_name[:16]
                
                header = bytearray(60)
                # Name (16 bytes, space-padded)
                header[0:16] = member_name.ljust(16).encode('ascii')
                # Timestamp (12 bytes)
                header[16:28] = str(current_time).ljust(12).encode('ascii')
                # UID (6 bytes) - use 0
                header[28:34] = b'0     '
                # GID (6 bytes) - use 0
                header[34:40] = b'0     '
                # Mode (8 bytes) - use 644 in octal
                header[40:48] = b'100644  '
                # Size (10 bytes)
                header[48:58] = str(data_size).ljust(10).encode('ascii')
                # Magic (2 bytes)
                header[58:60] = b'`\n'
                
                # Write header
                ar_file.write(bytes(header))
                
                # Write data
                ar_file.write(der_data)
                
                # CRITICAL: Add padding if size is odd (BSD ar requirement)
                if data_size % 2 == 1:
                    ar_file.write(b'\n')
                    print(f"    Added padding byte for {member_name} (size={data_size})", file=sys.stderr)

        print(f"✅ Created BSD archive: {output_path} with {added_count} certificates", file=sys.stderr)
        
        # Verify the archive
        print("\nVerifying archive integrity...", file=sys.stderr)
        if verify_ar_archive(output_path):
            print("✅ Archive verification PASSED", file=sys.stderr)
        else:
            print("⚠️  Archive verification had warnings", file=sys.stderr)

        if not keep_der:
            # Clean up temporary directory
            shutil.rmtree(der_dir, ignore_errors=True)
            print("\nTemporary DER files removed.", file=sys.stderr)
        else:
            print(f"\nDER files preserved in: {os.path.abspath(der_dir)}", file=sys.stderr)
            print("You can verify them with: openssl x509 -inform DER -in <file> -text -noout", file=sys.stderr)

        return True

    except Exception as e:
        print(f"Error creating archive: {e}", file=sys.stderr)
        import traceback
        traceback.print_exc()
        if not keep_der and 'der_dir' in locals():
            shutil.rmtree(der_dir, ignore_errors=True)
        return False

def verify_ar_archive(archive_path):
    """Verify that ar archive is properly formatted and all certificates are readable."""
    try:
        with open(archive_path, 'rb') as f:
            # Check magic
            magic = f.read(8)
            if magic != b"!<arch>\n":
                print(f"  ❌ Invalid ar magic: {magic}", file=sys.stderr)
                return False
            
            cert_count = 0
            while True:
                # Read header (60 bytes)
                header = f.read(60)
                if len(header) == 0:
                    break  # End of file
                
                if len(header) != 60:
                    print(f"  ❌ Incomplete header at position {f.tell() - len(header)}", file=sys.stderr)
                    return False
                
                # Parse header
                name = header[0:16].decode('ascii').rstrip()
                size_str = header[48:58].decode('ascii').strip()
                size = int(size_str)
                magic = header[58:60]
                
                if magic != b'`\n':
                    print(f"  ❌ Invalid member magic for {name}: {magic}", file=sys.stderr)
                    return False
                
                # Read data
                data = f.read(size)
                if len(data) != size:
                    print(f"  ❌ Incomplete data for {name}: expected {size}, got {len(data)}", file=sys.stderr)
                    return False
                
                # Verify it's a valid certificate
                try:
                    cert = x509.load_der_x509_certificate(data)
                    cert_count += 1
                    # Also verify CA status (optional check)
                    ca_status = "CA" if is_ca_certificate(cert) else "EE"
                    print(f"  ✅ {name}: Valid {ca_status} certificate, size={size} bytes", file=sys.stderr)
                except Exception as e:
                    print(f"  ❌ {name}: Invalid certificate: {e}", file=sys.stderr)
                    return False
                
                # Skip padding if size is odd
                if size % 2 == 1:
                    padding = f.read(1)
                    if padding != b'\n':
                        print(f"  ⚠️  {name}: Expected padding byte, got {padding}", file=sys.stderr)
            
            print(f"\n  Total certificates verified: {cert_count}", file=sys.stderr)
            return True
            
    except Exception as e:
        print(f"  ❌ Archive verification error: {e}", file=sys.stderr)
        import traceback
        traceback.print_exc()
        return False

def create_ar_archive(certificates, output_path, keep_der=False):
    """
    Create ar archive - now with proper manual implementation.
    This replaces the arlib-based implementation to fix corruption issues.
    """
    return create_bsd_ar_archive_manual(certificates, output_path, keep_der)

def main():
    parser = argparse.ArgumentParser(
        description='Download certificates from multiple hosts and create certsstore (ar archive) containing only CA certificates (root and intermediate).',
        epilog='Example: %(prog)s -s google.com -s github.com -o certs.ar --keep-der'
    )
    parser.add_argument('-s', '--servers', action='append', required=True,
                       help='TLS server(s) in format host[:port]. Can be specified multiple times.')
    parser.add_argument('-o', '--output', default='certsstore.ar',
                       help='Output archive file (default: certsstore.ar)')
    parser.add_argument('-v', '--verbose', action='store_true',
                       help='Verbose output')
    parser.add_argument('--keep-der', action='store_true',
                       help='Keep DER files in ./der_certs/ for manual inspection')
    parser.add_argument('--verify', action='store_true',
                       help='Only verify existing archive, do not create new one')
    parser.add_argument('--include-end-entity', action='store_true',
                       help='Include end-entity (leaf) certificates as well (default: only CA certificates)')

    args = parser.parse_args()

    # If verify mode, just verify and exit
    if args.verify:
        if not os.path.exists(args.output):
            print(f"Error: Archive {args.output} does not exist", file=sys.stderr)
            return 1
        
        print(f"Verifying archive: {args.output}", file=sys.stderr)
        if verify_ar_archive(args.output):
            print("\n✅ Archive is valid", file=sys.stderr)
            return 0
        else:
            print("\n❌ Archive verification failed", file=sys.stderr)
            return 1

    print(f"Generating certificate archive: {args.output}", file=sys.stderr)
    print(f"Timestamp: {datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')}", file=sys.stderr)
    print(file=sys.stderr)

    all_certificates = []
    unique_certs = set()

    for server_spec in args.servers:
        if ':' in server_spec:
            hostname, port_str = server_spec.split(':', 1)
            try:
                port = int(port_str)
            except ValueError:
                print(f"Warning: Invalid port for {server_spec}, using 443", file=sys.stderr)
                port = 443
        else:
            hostname = server_spec
            port = 443

        print(f"Fetching certificates from: {hostname}:{port}", file=sys.stderr)

        certs = get_certificates_from_host(hostname, port)

        for cert in certs:
            try:
                cert_der = cert.public_bytes(Encoding.DER)
                cert_hash = hashlib.sha256(cert_der).hexdigest()
                if cert_hash not in unique_certs:
                    unique_certs.add(cert_hash)
                    all_certificates.append(cert)
            except Exception as e:
                if args.verbose:
                    print(f"  Warning: {e}", file=sys.stderr)

        print(f"  Found {len(certs)} certificate(s), {len(all_certificates)} unique total", file=sys.stderr)
        print(file=sys.stderr)

    # Filter to only CA certificates unless --include-end-entity is given
    total_before_filter = len(all_certificates)
    if not args.include_end_entity:
        ca_certificates = [cert for cert in all_certificates if is_ca_certificate(cert)]
        filtered_count = total_before_filter - len(ca_certificates)
        all_certificates = ca_certificates
        print(f"Filtered out {filtered_count} non-CA certificates (end-entity).", file=sys.stderr)
        print(f"Remaining {len(all_certificates)} CA certificates.\n", file=sys.stderr)
    else:
        print("Including all certificates (CA and end-entity).\n", file=sys.stderr)

    if all_certificates:
        success = create_ar_archive(all_certificates, args.output, args.keep_der)
        if success:
            print(f"\n✅ Successfully created {args.output} with {len(all_certificates)} unique certificates", file=sys.stderr)
            print(f"\nTo verify: {sys.argv[0]} --verify -o {args.output}", file=sys.stderr)
            return 0
        else:
            return 1
    else:
        print("No CA certificates were found after filtering. Archive not created.", file=sys.stderr)
        return 1

if __name__ == '__main__':
    sys.exit(main())