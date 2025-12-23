# libdvdcss source files
dvdcss_src = files(
    'cpxm.c',
    'css.c',
    'device.c',
    'error.c',
    'ioctl.c',
    'libdvdcpxm.c',
    'libdvdcss.c',
)

#
# Library definitions
#

if host_machine.system() in ['windows', 'os/2'] and get_option('default_library') != 'static'
    api_export_flags = '-DLIBDVDCSS_EXPORTS'
else
    api_export_flags = []
endif

# The final libdvdcss library
libdvdcss = library('dvdcss', dvdcss_src,
    include_directories: dvdcss_inc_dirs,
    dependencies: extra_dependencies,
    c_args: [api_export_flags],
    gnu_symbol_visibility: 'hidden',
    version: dvdcss_soname_version,
    install: true,
)

# Make the library usable as a dependency in this and other projects
libdvdcss_dep = declare_dependency(link_with: libdvdcss,
    include_directories: include_directories('.'))

meson.override_dependency('libdvdcss', libdvdcss_dep)

# Generate pkg-config .pc file
pkg_mod = import('pkgconfig')
pkg_mod.generate(libraries: libdvdcss,
    version: meson.project_version(),
    name: 'libdvdcss',
    description: 'DVD access and decryption library',
)
