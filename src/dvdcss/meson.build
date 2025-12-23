dvdcss_api_headers = files(
    'dvdcpxm.h',
    'dvdcss.h',
)

dvdcss_api_headers += configure_file(input: 'version.h.in',
    output: 'version.h',
    configuration: {'DVDCSS_VERSION_MAJOR': dvdcss_version_split[0],
                    'DVDCSS_VERSION_MINOR': dvdcss_version_split[1],
                    'DVDCSS_VERSION_MICRO': dvdcss_version_split[2]})

# Install headers
install_headers(dvdcss_api_headers,
                subdir: 'dvdcss')
