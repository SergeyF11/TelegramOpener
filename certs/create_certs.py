from __future__ import print_function 
import csv 
import os 
import sys 
from cryptography import x509 
from cryptography.hazmat.primitives import serialization 
from cryptography.hazmat.backends import default_backend 
import shutil 
import arlib
 
try: 
    from urllib.request import urlopen 
except ImportError: 
    from urllib2 import urlopen 
 
try: 
    from io import StringIO 
except ImportError: 
    from StringIO import StringIO 
 
# Mozilla's URL for the CSV file with included PEM certs 
mozurl = "https://ccadb-public.secure.force.com/mozilla/IncludedCACertificateReportPEMCSV" 
 
# Load the names[] and pems[] array from the URL 
names = [] 
pems = [] 
response = urlopen(mozurl) 
csvData = response.read() 
if sys.version_info[0] > 2: 
    csvData = csvData.decode('utf-8') 
csvFile = StringIO(csvData) 
csvReader = csv.reader(csvFile) 
for row in csvReader: 
    names.append(row[0] + ":" + row[1] + ":" + row[2]) 
    for item in row: 
        if item.startswith("'-----BEGIN CERTIFICATE-----"): 
            pems.append(item) 
print("Remove header", names[0])            
del names[0] # Remove headers
if len(pems) - len(names) == 1:
    print("Remove header certificate", pems[0])
    del pems[0] # Remove headers 
 
# Try and make ./data, skip if present 
os.makedirs("data", exist_ok=True) 
 
derFiles = [] 
idx = 0 
 
# Convert PEM certificates to DER format 
for i in range(len(pems)): 
    certName = "data/ca_%03d.der" % idx 
    thisPem = pems[i].replace("'", "") 
    print(names[i] + " -> " + certName) 
     
    cert = x509.load_pem_x509_certificate(thisPem.encode('utf-8'), default_backend()) 
    der_data = cert.public_bytes(serialization.Encoding.DER) 
     
    with open(certName, "wb") as der_file: 
        der_file.write(der_data) 
     
    if os.path.exists(certName): 
        derFiles.append(certName) 
        idx += 1 
 
# Create an archive file manually 
archive_path = "data/certs.ar" 
if os.path.exists(archive_path): 
    os.remove(archive_path) 
 
#with open(archive_path, "wb") as archive: 
#    for der_file in derFiles: 
#        with open(der_file, "rb") as df: 
#            shutil.copyfileobj(df, archive) 

ar = arlib.Archive(format=arlib.BSD)
for der_file in derFiles:
     ar.add(der_file)
ar.save( archive_path )
 
# Clean up individual DER files 
for der in derFiles: 
    os.remove(der)

