#  Copyright (c) 2015 - 2024 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

prefix ?= /usr
datarootdir ?= $(prefix)/share
mandir ?= $(datarootdir)/man
sysconfdir ?= $(prefix)/etc
completionsdir ?= `pkg-config --variable=completionsdir bash-completion || echo $(sysconfdir)/bash_completion.d`


all:
	# Do not make anything by default
	-

json_schemas: json_schemas/active_sessions.schema.json json_schemas/const_config_io.schema.json

json_schemas/active_sessions.schema.json:
	PYTHONPATH=../geopmdpy \
	    python3 -c \
	    'from geopmdpy.schemas import GEOPM_ACTIVE_SESSIONS_SCHEMA as schema; print(schema)' > $@

json_schemas/const_config_io.schema.json:
	PYTHONPATH=../geopmdpy \
	    python3 -c \
	    'from geopmdpy.schemas import GEOPM_CONST_CONFIG_IO_SCHEMA as schema; print(schema)' > $@

man: html
	PYTHONPATH=../geopmdpy:../geopmpy \
	    sphinx-build -M man source build -W

html: VERSION json_schemas
	PYTHONPATH=../geopmdpy:../geopmpy \
	    sphinx-build -M html source build -W
	./doxygen_update.sh

# Nobody asked for this build target, so don't make it a build-blocker unless
# we decide to keep it. If we choose to keep it, then be sure to ratchet in all
# the errors, add -W back to the options, and add this as a dependency to the
# `docs` target
geopmlint:
	PYTHONPATH=../geopmdpy:../geopmpy \
	    sphinx-build -M geopmlint source build

clean_all:
	rm -rf build VERSION DATE debian/changelog geopm-doc-* geopm-doc.spec dox/libgeopmd dox/libgeopm source/_ext/__pycache__

VERSION:
	python3 -c "from setuptools_scm import get_version; print(get_version('..'))" > $@

DATE:
	date +'%a, %d %b %Y %H:%M:%S %z' > $@

MANIFEST:
	git ls-tree --full-tree --name-only -r HEAD | grep '^docs/' > MANIFEST.partial
	ls source/*.[1-9].rst | sed -e 's|source/\(.*\)\.rst|docs/build/man/\1|' >> MANIFEST.partial
	echo docs/VERSION >> MANIFEST.partial
	echo docs/DATE >> MANIFEST.partial
	echo docs/debian/changelog >> MANIFEST.partial
	echo docs/geopm-doc.spec >> MANIFEST.partial
	sort MANIFEST.partial > $@
	rm MANIFEST.partial

dist: man VERSION DATE MANIFEST
	VERSION=`cat VERSION` && DATE=`cat DATE` && \
	cat debian/changelog.in | sed -e "s|@VERSION@|$$VERSION|g" \
	                              -e "s|@DATE@|$$DATE|g" > debian/changelog && \
	sed -e "s|@ARCHIVE@|geopm-doc-$$VERSION|" -e "s|@VERSION@|$$VERSION|" geopm-doc.spec.in > geopm-doc.spec && \
	cd .. && tar -T docs/MANIFEST --transform="s|docs|geopm-doc-$$VERSION|" -h -zcvf geopm-doc-$$VERSION.tar.gz && \
	mv geopm-doc-$$VERSION.tar.gz docs

install_man:
	install -d $(DESTDIR)$(mandir)/man1
	install -d $(DESTDIR)$(mandir)/man3
	install -d $(DESTDIR)$(mandir)/man7
	install -m644 build/man/*.1 $(DESTDIR)$(mandir)/man1
	install -m644 build/man/*.3 $(DESTDIR)$(mandir)/man3
	install -m644 build/man/*.7 $(DESTDIR)$(mandir)/man7
	rm -f $(DESTDIR)$(mandir)/man1/geopmbench.1
	rm -f $(DESTDIR)$(mandir)/man1/geopmendpoint.1
	rm -f $(DESTDIR)$(mandir)/man3/geopm::Comm.3
	rm -f $(DESTDIR)$(mandir)/man3/geopm::Daemon.3
	rm -f $(DESTDIR)$(mandir)/man3/geopm::Endpoint.3
	rm -f $(DESTDIR)$(mandir)/man3/geopm::GPUActivityAgent.3
	rm -f $(DESTDIR)$(mandir)/man3/geopm::MPIComm.3
	rm -f $(DESTDIR)$(mandir)/man3/geopm::MonitorAgent.3
	rm -f $(DESTDIR)$(mandir)/man3/geopm::PowerBalancerAgent.3
	rm -f $(DESTDIR)$(mandir)/man3/geopm::PowerGovernorAgent.3
	rm -f $(DESTDIR)$(mandir)/man3/geopm::ProfileIOGroup.3
	rm -f $(DESTDIR)$(mandir)/man3/geopm_daemon.3
	rm -f $(DESTDIR)$(mandir)/man3/geopm_endpoint.3
	rm -f $(DESTDIR)$(mandir)/man3/geopm_fortran.3
	rm -f $(DESTDIR)$(mandir)/man3/geopm_policystore.3
	rm -f $(DESTDIR)$(mandir)/man7/geopm_agent_cpu_activity.7
	rm -f $(DESTDIR)$(mandir)/man7/geopm_agent_gpu_activity.7

install_completion:
	install -d $(DESTDIR)$(completionsdir)
	install -m644 shell_completion/geopmlaunch.bash $(DESTDIR)$(completionsdir)/geopmlaunch
	install -m644 shell_completion/geopmread.bash $(DESTDIR)$(completionsdir)/geopmread
	install -m644 shell_completion/geopmwrite.bash $(DESTDIR)$(completionsdir)/geopmwrite

install_html:
	if [ ! -d "$(GEOPM_GITHUB_IO)" ]; then echo "Error: Set GEOPM_GITHUB_IO variable to the path of the web page git repo" 1>&2; exit -1; fi
	cp -rp build/html/* $(GEOPM_GITHUB_IO)
	cp -rp json_schemas/* $(GEOPM_GITHUB_IO)
	VERSION=`cat VERSION` && cd $(GEOPM_GITHUB_IO) && git add -A && git commit -sm"Update to version $$VERSION"

.PHONY: all man html geopmlint clean_all DATE MANIFEST dist install_man install_completion install_html json_schemas json_schemas/active_sessions.schema.json json_schemas/const_config_io.schema.json
