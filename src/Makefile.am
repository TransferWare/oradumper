SUBDIRS    = lib prog

lintdirs   = lib prog

lint:
	@echo Making $@ in $(lintdirs)
	@for d in $(lintdirs); do (cd $$d && make $@); done


