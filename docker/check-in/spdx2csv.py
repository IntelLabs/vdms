import csv
import argparse

header=['Package', 'Version', 'License', 'Package Supplier', 'SPDXID']


def get_parameters():
    obj = argparse.ArgumentParser()
    obj.add_argument('-i', type=str, dest='INPUT_FILE',
                     default='docker/check-in/vdms_docker_sbom.txt',
                     help='Path to SBOM')
    obj.add_argument('-o', type=str, dest='OUTPUT_FILE',
                     default='docker/check-in/vdms_docker_sbom.csv',
                     help='Path to output SBOM as CSV')
    
    params = obj.parse_args()
    return params


def remove_newline(line):
    if "\n" in line:
        return line.replace("\n","")
    return line


def main(args):
    output_fh = open(args.OUTPUT_FILE, 'w', newline='', encoding='utf-8')
    csv_writer = csv.writer(output_fh)
    csv_writer.writerow(header)
    
    rows = []
    default_val = ""    
    with open(args.INPUT_FILE, 'r') as fh:
        # Skip File info
        for line in fh:
            if line in ['\n','\r\n']:
                break
        
        # Parse remaining lines    
        for line in fh:
            pkg_str = "##### Package: "
            if line.startswith(pkg_str):
                package_name = remove_newline(line[len(pkg_str):])
            
            ver_str = "PackageVersion: "
            if line.startswith(ver_str):
                version_num = remove_newline(line[len(ver_str):])
            
            lic_str = "PackageLicenseConcluded: "
            if line.startswith(lic_str):
                license_names = remove_newline(line[len(lic_str):])
            
            extref_str = "ExternalRef: PACKAGE_MANAGER purl pkg:"
            if line.startswith(extref_str):            
                package_type = remove_newline(line.split("/")[0].replace(extref_str,""))
                # row =  ",".join([package_name, version_num, license_names, package_type, spdxid])
                rows.append([package_name, version_num, license_names, package_type, spdxid])
                package_name, version_num, license_names, package_type, spdxid = default_val, default_val, default_val, default_val, default_val
                
            spdxid_str = "SPDXID: "
            if line.startswith(spdxid_str):
                spdxid = remove_newline(line[len(spdxid_str):])

        # Write rows
        csv_writer.writerows(rows)
    
    # Close output file
    output_fh.close()


if __name__ == '__main__':
    args = get_parameters()
    main(args)
