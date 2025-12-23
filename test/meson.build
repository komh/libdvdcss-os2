if not get_option('enable_examples')
    subdir_done()
endif

executable('csstest', 'csstest.c',
    dependencies: libdvdcss_dep)

executable('dvd_region', 'dvd_region.c',
    dependencies: libdvdcss_dep,
    include_directories: dvdcss_inc_dirs)
