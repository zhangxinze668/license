import license_validator

def main():
    path_to_license = './license.txt'
    try:
        license_validator.check_license(path_to_license)
        print('License is valid')
    except license_validator.LicenseError as e:
        print('License is invalid:', e)
        
if __name__=='__main__':
    main()