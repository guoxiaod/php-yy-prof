NAME=php-yy-prof
VERSION=$(shell awk '/^Version:/ {print $$2;exit;}' ${NAME}.spec)
SRC_FILE=${NAME}-${VERSION}.tgz
SRC_URL=http://pecl.php.net/get/${SRC_FILE}
ARCH=$(shell uname -m)

RPMBUILD_ROOT=${HOME}/rpmbuild
RPMBUILD_SPEC=rpmbuild -bb

all: fetch build copy


fetch:
	rm -rf ${NAME}-${VERSION} && cp -r src ${NAME}-${VERSION}
	tar -czf ${SRC_FILE} ${NAME}-${VERSION}
	rm -rf ${NAME}-${VERSION}

build:
	cp ${SRC_FILE} ${RPMBUILD_ROOT}/SOURCES/
	${RPMBUILD_SPEC} ${NAME}.spec ${RPMBUILD_OPTIONS}
	
copy:
	cp ${RPMBUILD_ROOT}/RPMS/${ARCH}/*${NAME}*${VERSION}*.rpm .

