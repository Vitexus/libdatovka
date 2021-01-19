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

DESIRED_BRANCH=""
DESIRED_COMMIT=""
DESIRED_VERSION=""

B_SHORT="-b"
B_LONG="--branch"

C_SHORT="-c"
C_LONG="--commit"

H_SHORT="-h"
H_LONG="--help"

V_SHORT="-v"
V_LONG="--version-tag"

USAGE="Usage:\n\t$0 [options] [version]\n\n"
USAGE="${USAGE}Supported options:\n"
USAGE="${USAGE}\t${B_SHORT}, ${B_LONG} <branch>\n\t\tSpecify branch to take sources from.\n"
USAGE="${USAGE}\t${C_SHORT}, ${C_LONG} <commit>\n\t\tSpecify commit to take sources from.\n"
USAGE="${USAGE}\t${H_SHORT}, ${H_LONG}\n\t\tPrints help message.\n"
USAGE="${USAGE}\t${V_SHORT}, ${V_LONG} <version>\n\t\tSpecify version (i.e. tag without leading 'v').\n"

# Parse rest of command line
while [ $# -gt 0 ]; do
	KEY="$1"
	VAL="$2"
	case "${KEY}" in
	${B_SHORT}|${B_LONG})
		if [ "x${VAL}" = "x" ]; then
			echo "Option '${KEY}' requires a parameter." >&2
			exit 1
		fi
		if [ "x${DESIRED_COMMIT}" != "x" -o "x${DESIRED_VERSION}" != "x" ]; then
			echo "You cannot specify a branch together with a commit or version." >&2
			exit 1
		elif [ "x${DESIRED_BRANCH}" = "x" ]; then
			DESIRED_BRANCH="${VAL}"
			shift
		else
			echo "Branch already specified or in conflict." >&2
			exit 1
		fi
		;;
	${C_SHORT}|${C_LONG})
		if [ "x${VAL}" = "x" ]; then
			echo "Option '${KEY}' requires a parameter." >&2
			exit 1
		fi
		if [ "x${DESIRED_BRANCH}" != "x" -o "x${DESIRED_VERSION}" != "x" ]; then
			echo "You cannot specify a commit together with a branch or version." >&2
			exit 1
		elif [ "x${DESIRED_COMMIT}" = "x" ]; then
			DESIRED_COMMIT="${VAL}"
			shift
		else
			echo "Commit already specified or in conflict." >&2
			exit 1
		fi
		;;
	${H_SHORT}|${H_LONG})
		echo -e ${USAGE}
		exit 0
		;;
	${V_SHORT}|${V_LONG})
		if [ "x${VAL}" = "x" ]; then
			echo "Option '${KEY}' requires a parameter." >&2
			exit 1
		fi
		if [ "x${DESIRED_BRANCH}" != "x" -o "x${DESIRED_COMMIT}" != "x" ]; then
			echo "You cannot specify a version together with a branch or commit." >&2
			exit 1
		elif [ "x${DESIRED_VERSION}" = "x" ]; then
			DESIRED_VERSION="${VAL}"
			shift
		else
			echo "Version already specified or in conflict." >&2
			exit 1
		fi
		;;
	--)
		shift
		break
		;;
	-*|*)
		# Unknown option.
		break
		;;
	esac
	shift
done
if [ $# -eq 1 ]; then
	if [ "x${DESIRED_VERSION}" != "x" ]; then
		echo -e "Version tag is already set to '${DESIRED_VERSION}'." >&2
		echo -en ${USAGE} >&2
		exit 1
	fi
	DESIRED_VERSION="$1"
elif [ $# -gt 1 ]; then
	echo -e "Unknown options: $@" >&2
	echo -en ${USAGE} >&2
	exit 1
fi

if [ "x${DESIRED_BRANCH}" != "x" -a "x${DESIRED_VERSION}" != "x" ]; then
	echo "You cannot specify both branch and tag." >&2
	exit 1
fi

LATEST_AVAILABLE_VERSION=""
ALL_AVAILABLE_TAGS=""
#DESIRED_BRANCH=""
DESIRED_TAG=""
#DESIRED_VERSION""

if [ "x${DESIRED_BRANCH}" != "x" ]; then
	# Use latest commit in specified branch.
	ALL_BRANCHES=$(git branch -a)

	FOUND="no"
	for B in ${ALL_BRANCHES}; do
		if [ "${B}" = "${DESIRED_BRANCH}" ]; then
			FOUND="yes"
		fi
	done
	if [ "${FOUND}" = "no" ]; then
		echo "Branch '${DESIRED_BRANCH}' not found." >&2
		exit 1
	fi

	ALL_AVAILABLE_TAGS=$(git tag -l --sort=-version:refname --merged "${DESIRED_BRANCH}")
	LATEST_TAG=$(echo "${ALL_AVAILABLE_TAGS}" | head -n 1)
	LATEST_VERSION=$(echo "${LATEST_TAG}" | sed -e 's/^v//g')
	if [ "x${LATEST_VERSION}" = "x" ]; then
		echo "Cannot determine latest version tag." >&2
		exit 1
	fi

	DESIRED_VERSION="${LATEST_VERSION}"

	echo "Building archive version '${DESIRED_VERSION}' from branch '${DESIRED_BRANCH}'."
elif [ "x${DESIRED_COMMIT}" != "x" ]; then
	# Use specified commit.

	COMMIT=$(git cat-file -t "${DESIRED_COMMIT}")
	if [ "x${COMMIT}" != "xcommit" ]; then
		echo "Commit '${DESIRED_COMMIT}' not found." >&2
		exit 1
	fi

	ALL_AVAILABLE_TAGS=$(git tag -l --sort=-version:refname --merged "${DESIRED_COMMIT}")
	LATEST_TAG=$(echo "${ALL_AVAILABLE_TAGS}" | head -n 1)
	LATEST_VERSION=$(echo "${LATEST_TAG}" | sed -e 's/^v//g')
	if [ "x${LATEST_VERSION}" = "x" ]; then
		echo "Cannot determine latest version tag." >&2
		exit 1
	fi

	DESIRED_VERSION="${LATEST_VERSION}"

	echo "Building archive version '${DESIRED_VERSION}' from commit '${DESIRED_COMMIT}'."
else
	# Use latest or specified version.
	ALL_AVAILABLE_TAGS=$(git tag -l --sort=-version:refname)
	LATEST_TAG=$(echo "${ALL_AVAILABLE_TAGS}" | head -n 1)
	LATEST_VERSION=$(echo "${LATEST_TAG}" | sed -e 's/^v//g')
	if [ "x${LATEST_VERSION}" = "x" ]; then
		echo "Cannot determine latest version tag." >&2
		exit 1
	fi

	if [ "x${DESIRED_VERSION}" = "x" ]; then
		DESIRED_VERSION="${LATEST_VERSION}"
	fi
	DESIRED_TAG="v${DESIRED_VERSION}"

	FOUND="no"
	for T in ${ALL_AVAILABLE_TAGS}; do
		if [ "${T}" = ${DESIRED_TAG} ]; then
			FOUND="yes"
		fi
	done
	if [ "${FOUND}" = "no" ]; then
		echo "Tag '${DESIRED_TAG}' of desired version '${DESIRED_VERSION}' not found." >&2
		exit 1
	fi

	echo "Building archive version '${DESIRED_VERSION}' from tag '${DESIRED_TAG}'."
fi

archive_override_version() {
	local ARCHIVE="$1"
	local PACKAGE_NAME="$2"
	local NEW_VERSION="$3"

	if [ "x${ARCHIVE}" = "x" ]; then
		echo "No archive specified." >&2
		return 1
	fi
	if [ ! -f "${ARCHIVE}" ]; then
		echo "Supplied archive name '${ARCHIVE}' is not a file." >&2
		return 1
	fi

	if [ "x${PACKAGE_NAME}" = "x" ]; then
		echo "No package name specified." >&2
		return 1
	fi

	if [ "x${NEW_VERSION}" = "x" ]; then
		echo "No version specified." >&2
		return 1
	fi

	local TEMPDIR=$(mktemp -d tmp.XXXXXXXX)
	pushd "${TEMPDIR}" > /dev/null

	#tar -xf "../${ARCHIVE}" "${PACKAGE_NAME}/datovka.pro" "${PACKAGE_NAME}/pri/version.pri"
	tar -xf "../${ARCHIVE}"

	echo "VERSION = ${NEW_VERSION}" > "${PACKAGE_NAME}/pri/version.pri"

	# Modify timestamps of generated files in order to be able
	# to generate reproducible checksums.
	touch -r "${PACKAGE_NAME}/datovka.pro" "${PACKAGE_NAME}/pri/version.pri"

	# On macOS tar does not support the --delete option.
	# A completely new package must be created.
	#tar --delete -f "../${ARCHIVE}" "${PACKAGE_NAME}/pri/version.pri"
	#tar --append -f "../${ARCHIVE}" "${PACKAGE_NAME}/pri/version.pri"
	rm "../${ARCHIVE}" || return 1
	tar -cf "../${ARCHIVE}" "${PACKAGE_NAME}"

	popd > /dev/null
	rm -r "${TEMPDIR}"
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

COMMIT_ID=""
PACKAGE_NAME=""
TARGTET_TAR=""
TARGET_COMPRESSED=""

if [ "x${DESIRED_BRANCH}" != "x" ]; then
	COMMIT_ID=$(git rev-parse "${DESIRED_BRANCH}")
	SHORT_COMMIT_ID=$(git rev-parse --short=16 "${DESIRED_BRANCH}")
	#UTC_TIME=$(date -u +%Y%m%d.%H%M%S)
	UTC_TIME=$(TZ=UTC git show --quiet --date='format-local:%Y%m%d.%H%M%S' --format="%cd" "${DESIRED_BRANCH}")
	DESIRED_VERSION="${DESIRED_VERSION}.9999.${UTC_TIME}.${SHORT_COMMIT_ID}"
	PACKAGE_NAME="libdatovka-${DESIRED_VERSION}"
	TARGTET_TAR="${PACKAGE_NAME}.tar"
	TARGET_COMPRESSED="${PACKAGE_NAME}.tar.xz"
elif [ "x${DESIRED_COMMIT}" != "x" ]; then
	COMMIT_ID=$(git rev-parse "${DESIRED_COMMIT}")
	SHORT_COMMIT_ID=$(git rev-parse --short=16 "${DESIRED_COMMIT}")
	#UTC_TIME=$(date -u +%Y%m%d.%H%M%S)
	UTC_TIME=$(TZ=UTC git show --quiet --date='format-local:%Y%m%d.%H%M%S' --format="%cd" "${DESIRED_COMMIT}")
	DESIRED_VERSION="${DESIRED_VERSION}.9999.${UTC_TIME}.${SHORT_COMMIT_ID}"
	PACKAGE_NAME="libdatovka-${DESIRED_VERSION}"
	TARGTET_TAR="${PACKAGE_NAME}.tar"
	TARGET_COMPRESSED="${PACKAGE_NAME}.tar.xz"
elif [ "x${DESIRED_TAG}" != "x" ]; then
	COMMIT_ID=$(git rev-list -n 1 "${DESIRED_TAG}")
	PACKAGE_NAME="libdatovka-${DESIRED_VERSION}"
	TARGTET_TAR="${PACKAGE_NAME}.tar"
	TARGET_COMPRESSED="${PACKAGE_NAME}.tar.xz"
else
	echo "Cannot build git archive." >&2
	exit 1
fi

if [ "x${COMMIT_ID}" = "x" ]; then
	echo "Could not determine commit identifier." >&2
	exit 1
fi

rm -f "${TARGTET_TAR}" "${TARGET_COMPRESSED}"

# Outside git repository.
TEMPDIR=$(mktemp -d -t tmp.XXXXXXXX)
#popd > /dev/null
#rm -r "${TEMPDIR}"

# Generate state copy of the git repository.
if [ "x${DESIRED_BRANCH}" != "x" ]; then
	# Build from branch.
	git archive --format=tar "${DESIRED_BRANCH}" | tar -x -C "${TEMPDIR}"
elif [ "x${DESIRED_COMMIT}" != "x" ]; then
	# Build from commit.
	git archive --format=tar "${DESIRED_COMMIT}" | tar -x -C "${TEMPDIR}"
elif [ "x${DESIRED_TAG}" != "x" ]; then
	# Build from tag.
	git archive --format=tar "${DESIRED_TAG}" | tar -x -C "${TEMPDIR}"
fi

# Override version in the specified directory.
directory_override_version() {
	local DIR="$1"
	local VER="$2"
	if [ "x${DIR}" = "x" -o "x${VER}" = "x" ]; then
		return 1
	fi
	if [ ! -d "${DIR}" ]; then
		return 1
	fi

	sed -e "s/^AC_INIT.*$/AC_INIT([libdatovka], [${VER}], [datove-schranky@labs.nic.cz])/g" -i "${DIR}/configure.ac"
	# Has to be \\\\n beacuse of double quotes.
	sed -e "s/^.*Project-Id-Version:.*$/\"Project-Id-Version: libdatovka ${VER}\\\\n\"/g" -i "${DIR}/po/cs.po"
	sed -e "s/^.*Project-Id-Version:.*$/\"Project-Id-Version: libdatovka ${VER}\\\\n\"/g" -i "${DIR}/po/libdatovka.pot"

	# Add news entry if no corresponding version found.
	FOUND_NEWS=$(cat "${DIR}/NEWS" | grep "${VER}")
	if [ "x${FOUND_NEWS}" = "x" ]; then
		echo "${VER} /\n" | cat - "${DIR}/NEWS" > "${DIR}/NEWS.tmp" && mv "${DIR}/NEWS.tmp" "${DIR}/NEWS"
	fi
}

directory_override_version "${TEMPDIR}" "${DESIRED_VERSION}"

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

pushd "${TEMPDIR}" > /dev/null

if ! rebuild_configure; then
	echo "Cannot build ./configure script." >&2
	exit 1
fi

if ! configure_sources; then
	echo "Cannot generate Makefile file." >&2
	exit 1
fi

if ! run_distcheck; then
	echo "Cannot generate source archive." >&2
	exit 1
fi

TARGET_COMPRESSED=$(find_package_file "libdatovka" "${DESIRED_VERSION}")

popd > /dev/null

if [ -f "${TEMPDIR}/${TARGET_COMPRESSED}" ]; then
	mv "${TEMPDIR}/${TARGET_COMPRESSED}" ./
else
	echo "Cannot find archive." >&2
fi

rm -r "${TEMPDIR}"

compute_checksum "${TARGET_COMPRESSED}"

echo "${TARGET_COMPRESSED}"
