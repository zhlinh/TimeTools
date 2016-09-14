EXCLUDE_DIRS := Ptpd

DIRS := $(shell find . -maxdepth 1 -type d)

DIRS := $(basename $(patsubst ./%,%,$(DIRS)))

DIRS := $(filter-out $(EXCLUDE_DIRS), $(DIRS))

SUBDIRS := $(DIRS)

define make_subdir
@for subdir in $(SUBDIRS); do \
(cd $$subdir && make $1) \
done;
endef

all:
	@echo $(MAKE)...
	$(call make_subdir, all)

clean:
	@echo $(MAKE)...
	$(call make_subdir, clean)

.PHONY: SUBDIRS $(SUBDIRS) all clean

