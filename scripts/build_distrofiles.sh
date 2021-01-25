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

. "${SRC_ROOT}"/scripts/helper_dependency_sources.sh
. "${SRC_ROOT}"/scripts/helper_packaging.sh

PACKAGE=""
VERSION=""

H_SHORT="-h"
H_LONG="--help"

P_SHORT="-p"
P_LONG="--package"
P_LIBDATOVKA="libdatovka"
V_SHORT="-v"
V_LONG="--version"

USAGE="Usage:\n\t$0 [options]\n\n"
USAGE="${USAGE}Supported options:\n"
USAGE="${USAGE}\t${H_SHORT}, ${H_LONG}\n\t\tPrints help message.\n"
USAGE="${USAGE}\t${P_SHORT}, ${P_LONG} <package>\n\t\tSupported arguments are '${P_LIBDATOVKA}''.\n"
USAGE="${USAGE}\t${V_SHORT}, ${V_LONG} <version>\n\t\tExplicitly specify package version (applies only to ${P_LIBDATOVKA}).\n"

# Parse rest of command line
while [ $# -gt 0 ]; do
	KEY="$1"
	VAL="$2"
	case "${KEY}" in
	${H_SHORT}|${H_LONG})
		echo -e ${USAGE}
		exit 0
		;;
	${P_SHORT}|${P_LONG})
		if [ "x${VAL}" = "x" ]; then
			echo "Argument '${KEY}' requires an argument." >&2
			exit 1
		fi
		if [ "x${PACKAGE}" = "x" ]; then
			PACKAGE="${VAL}"
			shift
		else
			echo "Package name already specified or in conflict." >&2
			exit 1
		fi
		;;
	${V_SHORT}|${V_LONG})
		if [ "x${VAL}" = "x" ]; then
			echo "Argument '${KEY}' requires an argument." >&2
			exit 1
		fi
		if [ "x${VERSION}" = "x" ]; then
			VERSION="${VAL}"
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
		echo "Unknown option '${KEY}'." >&2
		echo -e ${USAGE} >&2
		exit 1
		;;
	esac
	shift
done
if [ $# -gt 0 ]; then
	echo -e "Unknown options: $@" >&2
	echo -en ${USAGE} >&2
	exit 1
fi

#PACKAGE=""
#VERSION=""
RELEASE=""

PACKAGE_SRC=""

# Set package to be uploaded.
case "x${PACKAGE}" in
x${P_LIBDATOVKA})
	if [ "x${VERSION}" != "x" ]; then
		if [ ! -f "${PACKAGE}-${VERSION}.tar.xz" ]; then
			echo "Missing file '${PACKAGE}-${VERSION}.tar.xz'." >&2
			exit 1
		fi
	else
		# Use latest release as default.
		VERSION="0.1.0"

		ensure_source_presence "${SRC_ROOT}" "${PACKAGE}-${VERSION}.tar.xz" \
		    "https://secure.nic.cz/files/datove_schranky/${VERSION}/" "a3fd28ef44bdc52ded29ba0a556af0cde1155f7eedcb895d586a260bdc81d25e" "" "" || exit 1
	fi

	RELEASE="1"
	PACKAGE_SRC="${SRC_ROOT}/${PACKAGE}-${VERSION}.tar.xz"
	;;
x)
	echo "Unspecified package." >&2
	exit 1
	;;
*)
	echo "Unsupported package '${PACKAGE}'." >&2
	exit 1
	;;
esac

file_present "${PACKAGE_SRC}" || exit 1

DISTRO_WORK_DIR="_distrofiles/${PACKAGE}"
rm_and_create_dir "${DISTRO_WORK_DIR}" || exit 1

cp -ra "${SRC_ROOT}/distro/${PACKAGE}/"* "${DISTRO_WORK_DIR}"
rm_swp_files "${DISTRO_WORK_DIR}"

fill_in_files "${DISTRO_WORK_DIR}" || exit 1

# Rename archive to Debian format.
cp -p "${PACKAGE_SRC}" "${DISTRO_WORK_DIR}/${PACKAGE}_${VERSION}.orig.tar.xz" || exit 1

# Create Debian archive and complete dsc.
pushd "${SRC_ROOT}/${DISTRO_WORK_DIR}/deb"
ARCHIVE="${PACKAGE}_${VERSION}-${RELEASE}.debian.tar.xz"
tar --mtime="2018-04-06 12:00Z" -chJf "${ARCHIVE}" debian
echo " $(compute_md5_checksum ${ARCHIVE}) $(wc -c ${ARCHIVE})" >> "${PACKAGE}.dsc"
cd ..
ARCHIVE="${PACKAGE}_${VERSION}.orig.tar.xz"
echo " $(compute_md5_checksum ${ARCHIVE}) $(wc -c ${ARCHIVE})" >> "deb/${PACKAGE}.dsc"
popd
