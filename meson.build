project('redis_pump', 'c', default_options : ['c_std=c11'])

thread_dep = dependency('threads')
executable('shm_reader_uds', 'shm_reader_uds.c', dependencies : thread_dep)
executable('shm_writer', 'shm_writer.c')

