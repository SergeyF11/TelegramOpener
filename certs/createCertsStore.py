#!/usr/bin/env python3

# Script to download certificates from multiple hosts
# and generate certsstore (ar archive with DER certificates).
# released to public domain

import urllib.request
import re
import ssl
import sys
import socket
import argparse
import datetime
import hashlib
import os
import tempfile
import subprocess
from pathlib import Path

from cryptography import x509
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.serialization import Encoding
from cryptography.hazmat.primitives.serialization import PublicFormat

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

def create_ar_archive(certificates, output_path):
    """Create ar archive from DER certificates."""
    if not certificates:
        print("No certificates to archive", file=sys.stderr)
        return False
    
    # Create temporary directory for DER files
    with tempfile.TemporaryDirectory() as temp_dir:
        der_files = []
        
        # Save certificates in DER format
        for i, cert in enumerate(certificates):
            try:
                # Generate filename from certificate properties
                cn = ""
                try:
                    for dn in cert.subject.rfc4514_string().split(','):
                        keyval = dn.split('=')
                        if keyval[0] == 'CN':
                            cn = keyval[1]
                except:
                    cn = f"cert_{i}"
                
                # Clean filename
                name = re.sub('[^a-zA-Z0-9_-]', '_', cn)
                if not name:
                    name = f"cert_{i}"
                
                # Add hash to avoid collisions
                cert_hash = hashlib.sha256(cert.public_bytes(Encoding.DER)).hexdigest()[:8]
                filename = f"{name}_{cert_hash}.der"
                filepath = os.path.join(temp_dir, filename)
                
                # Save as DER
                with open(filepath, 'wb') as f:
                    f.write(cert.public_bytes(Encoding.DER))
                
                der_files.append(filepath)
                
                # Print certificate info
                print(f"  {filename}: CN={cn}, Valid: {cert.not_valid_before} to {cert.not_valid_after}", 
                      file=sys.stderr)
                
            except Exception as e:
                print(f"  Warning: Failed to process certificate {i}: {e}", file=sys.stderr)
        
        if not der_files:
            print("No valid certificates to archive", file=sys.stderr)
            return False
        
        # Create ar archive
        try:
            # Try using ar command
            subprocess.run(['ar', 'rcs', output_path] + der_files, 
                          check=True, capture_output=True)
            print(f"Created archive: {output_path} with {len(der_files)} certificates", 
                  file=sys.stderr)
            return True
        except (subprocess.CalledProcessError, FileNotFoundError):
            try:
                # Try using arlib as fallback
                import arlib
                ar = arlib.Archive(format=arlib.BSD)
                for der_file in der_files:
                    ar.add(der_file)
                ar.save(output_path)
                print(f"Created archive (using arlib): {output_path} with {len(der_files)} certificates",
                      file=sys.stderr)
                return True
            except ImportError:
                print("Error: Neither 'ar' command nor 'arlib' module found", file=sys.stderr)
                print("Install arlib: pip install arlib", file=sys.stderr)
                return False

def main():
    parser = argparse.ArgumentParser(
        description='Download certificates from multiple hosts and create certsstore (ar archive)'
    )
    parser.add_argument('-s', '--servers', action='append', required=True,
                       help='TLS server(s) in format host[:port]. Can be specified multiple times.')
    parser.add_argument('-o', '--output', default='certsstore.ar',
                       help='Output archive file (default: certsstore.ar)')
    parser.add_argument('-v', '--verbose', action='store_true',
                       help='Verbose output')
    
    args = parser.parse_args()
    
    print(f"Generating certificate archive: {args.output}", file=sys.stderr)
    print(f"Timestamp: {datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')}", file=sys.stderr)
    print(file=sys.stderr)
    
    all_certificates = []
    unique_certs = set()  # Track unique certificates by DER hash
    
    # Process each server
    for server_spec in args.servers:
        # Parse host:port
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
        
        # Add unique certificates
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
        
        print(f"  Found {len(certs)} certificate(s), {len(all_certificates)} unique total", 
              file=sys.stderr)
        print(file=sys.stderr)
    
    # Create archive
    if all_certificates:
        success = create_ar_archive(all_certificates, args.output)
        if success:
            print(f"\nSuccessfully created {args.output} with {len(all_certificates)} unique certificates", 
                  file=sys.stderr)
            return 0
        else:
            return 1
    else:
        print("No certificates were downloaded", file=sys.stderr)
        return 1

if __name__ == '__main__':
    sys.exit(main())
