if not get_option('enable_docs')
    subdir_done()
endif

doxygen = find_program('doxygen', required: true)

doxyfile_conf = configuration_data()
doxyfile_conf.set('abs_builddir', meson.current_build_dir())
doxyfile_conf.set('abs_top_srcdir', dvdcss_src_root)
doxyfile_conf.set('STRIP_FROM_PATH', f'@dvdcss_src_root@/src \\\n @dvdcss_build_root@/src')
doxyfile_conf.set('PACKAGE_NAME', meson.project_name())
doxyfile_conf.set('PACKAGE_VERSION', meson.project_version())

doxyfile = configure_file(
    input: 'doxygen.cfg.in',
    output: 'Doxyfile',
    configuration: doxyfile_conf,
)

custom_target('docs',
    depend_files: dvdcss_api_headers,
    command: [doxygen, '-q', doxyfile],
    output: ['html'],
    install: true,
    install_dir: get_option('datadir') / 'doc/libdvdcss',
    install_tag: 'doc',
)
