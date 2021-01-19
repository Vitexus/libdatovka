#!/usr/bin/env sh

# Obtain location of source root.
src_root () {
	local SCRIPT_LOCATION=""
	local SYSTEM=$(uname -s)
	if [ ! "x${SYSTEM}" = "xDarwin" ]; then
		local SCRIPT=$(readlink -f "$0")
		SCRIPT_LOCATION=$(dirname $(readlink -f "$0"))
	else
		SCRIPT_LOCATION=$(cd "$(dirname "$0")"; pwd)
	fi

	echo $(cd "$(dirname "${SCRIPT_LOCATION}")"; pwd)
}


SRC_ROOT=$(src_root)
cd "${SRC_ROOT}"

# Rebuild ./configure script.
rebuild_configure() {
	local LIBTOOLIZE=libtoolize
	if which glibtoolize 1>/dev/null 2>&1; then
		LIBTOOLIZE=glibtoolize
	fi

	autoheader && \
	${LIBTOOLIZE} -c --install && \
	aclocal -I m4 && \
	automake --add-missing --copy && \
	autoconf && \
	return 0

	return 1
}

# Get package version from configure script.
get_package_version() {
	local CONFIGURE_FILE="$1"
	if [ "x${CONFIGURE_FILE}" = "x" ]; then
		echo ""
		return 1
	fi
	if [ ! -e "${CONFIGURE_FILE}" ]; then
		echo ""
		return 1
	fi

	VERSION=$(eval $(cat "${CONFIGURE_FILE}" | grep PACKAGE_VERSION=); echo ${PACKAGE_VERSION})
	echo $VERSION
	return 0
}

# Configure sources to obtain Makefile.
configure_sources() {
	local CONFOPTS=""
	CONFOPTS="${CONFOPTS} --enable-debug"
	CONFOPTS="${CONFOPTS} --enable-openssl-backend"
	CONFOPTS="${CONFOPTS} --disable-fatalwarnings"
	CONFOPTS="${CONFOPTS} --enable-test"
	CONFOPTS="${CONFOPTS} --prefix=/usr/local --libdir=/usr/local/lib64"

	./configure ${CONFOPTS} && return 0

	return 1
}

# Build source archives.
run_distcheck() {
	make distcheck && return 0

	return 1
}

# Find xzip archive.
find_package_file() {
	local PACKAGE="$1"
	local VERSION="$2"

	ls | grep "^${PACKAGE}-${VERSION}.*xz$"
}

compute_checksum() {
	local FILE_NAME="$1"

	local CMD_SHA256SUM=sha256sum
	local CMD_OPENSSL=openssl
	local SUMSUFF=sha256

	if [ -z $(command -v "${CMD_SHA256SUM}") ]; then
		echo "Install '${CMD_SHA256SUM}' to be able to check checksum file." >&2
		CMD_SHA256SUM=""
	fi

	if [ -z $(command -v "${CMD_OPENSSL}") ]; then
		echo "Install '${CMD_OPENSSL}' to be able to check checksum file." >&2
		CMD_OPENSSL=""
	fi

	if [ "x${FILE_NAME}" = "x" ]; then
		echo "Supplied empty file name." >&2
		return 1
	fi

	SED=sed
	if [ $(uname) = "Darwin" ]; then
		# OS X version of sed does not recognise \s as white space
		# identifier .
		SED=gsed
	fi

	if [ "x${CMD_SHA256SUM}" != "x" -a "${CMD_OPENSSL}" != "x" ]; then
		SHA256SUM_SUM=$("${CMD_SHA256SUM}" "${FILE_NAME}" | "${SED}" -e 's/\s.*$//g')
		OPENSSL_SUM=$("${CMD_OPENSSL}" sha256 "${FILE_NAME}" | "${SED}" -e 's/^.*\s//g')

		# Compare checksums.
		if [ "x${SHA256SUM_SUM}" = "x${OPENSSL_SUM}" -a "x${SHA256SUM_SUM}" != "x" ]; then
			echo "Checksum comparison OK."
			echo "${SHA256SUM_SUM}" > "${FILE_NAME}.${SUMSUFF}"
		else
			echo "Checksums differ or are empty." >&2
			return 1
		fi
	elif [ "x${CMD_SHA256SUM}" != "x" ]; then
		"${CMD_SHA256SUM}" "${FILE_NAME}" | "${SED}" -e 's/\s.*$//g' > "${FILE_NAME}.${SUMSUFF}"
	elif [ "x${CMD_OPENSSL}" != "x" ]; then
		"${CMD_OPENSSL}" sha256 "${FILE_NAME}" | "${SED}" -e 's/^.*\s//g' > "${FILE_NAME}.${SUMSUFF}"
	else
		echo "No command to compute sha256 checksum." >&2
		return 1
	fi

	SUM_FILE_SIZE=""
	EXPECTED_SUM_FILE_SIZE="65"
	if [ $(uname) != "Darwin" ]; then
		SUM_FILE_SIZE=$(stat -c '%s' "${FILE_NAME}.${SUMSUFF}")
	else
		SUM_FILE_SIZE=$(stat -f '%z' "${FILE_NAME}.${SUMSUFF}")
	fi
	if [ "x${SUM_FILE_SIZE}" = "x${EXPECTED_SUM_FILE_SIZE}" ]; then
		echo "File size of '${FILE_NAME}.${SUMSUFF}' OK."
	else
		echo "Unexpected size '${SUM_FILE_SIZE}' of file '${FILE_NAME}.${SUMSUFF}'." >&2
		rm "${FILE_NAME}.${SUMSUFF}"
		return 1
	fi
}

if ! rebuild_configure; then
	echo "Cannot build ./configure script." >&2
	exit 1
fi

PACKAGE=libdatovka
PACKAGE_VERSION=$(get_package_version ./configure)

if ! configure_sources; then
	echo "Cannot generate Makefile file." >&2
	exit 1
fi

if ! run_distcheck; then
	echo "Cannot generate source archive." >&2
	exit 1
fi

TARGET_COMPRESSED=$(find_package_file "${PACKAGE}" "${PACKAGE_VERSION}")

if ! compute_checksum "${TARGET_COMPRESSED}"; then
	echo "Cannot compute checksum." >&2
	exit 1
fi

echo Ok
