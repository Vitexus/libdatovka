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

. "${SRC_ROOT}"/scripts/helper_generic.sh
. "${SRC_ROOT}"/scripts/helper_packaging.sh

REPO=""
PACKAGE=""
VERSION=""
ANSWER_YES="no"

H_SHORT="-h"
H_LONG="--help"

P_SHORT="-p"
P_LONG="--package"
P_LIBDATOVKA="libdatovka"

V_SHORT="-v"
V_LONG="--version"

D_LONG="--devel"
L_LONG="--latest"
Y_LONG="--yes"

USAGE="Usage:\n\t$0 [options]\n\n"
USAGE="${USAGE}Supported options:\n"
USAGE="${USAGE}\t${H_SHORT}, ${H_LONG}\n\t\tPrints help message.\n"
USAGE="${USAGE}\t${P_SHORT}, ${P_LONG} <package>\n\t\tSupported arguments are '${P_LIBDATOVKA}'.\n"
USAGE="${USAGE}\t${V_SHORT}, ${V_LONG} <version>\n\t\tExplicitly specify package version (applies only to ${P_LIBDATOVKA}).\n"
USAGE="${USAGE}\t${D_LONG} (default)\n\t\tPush into '${HP_REPO_DEVEL}'.\n"
USAGE="${USAGE}\t${L_LONG}\n\t\tPush into '${HP_REPO_LATEST}'.\n"
USAGE="${USAGE}\t${Y_LONG}\n\t\tAutomatically answer 'yes' to all questions.\n"

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
	${D_LONG})
		if [ "x${REPO}" = "x" ]; then
			REPO="${HP_REPO_DEVEL}"
		else
			echo "Subproject already specified or in conflict." >&2
			exit 1
		fi
		;;
	${L_LONG})
		if [ "x${REPO}" = "x" ]; then
			REPO="${HP_REPO_LATEST}"
		else
			echo "Subproject already specified or in conflict." >&2
			exit 1
		fi
		;;
	${Y_LONG})
		ANSWER_YES="yes"
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

# Use devel by default.
if [ "x${REPO}" = "x" ]; then
	REPO="${HP_REPO_DEVEL}"
fi

# Cannot use latest and explicitly specified version.
if [ "x${REPO}" != "x${HP_REPO_DEVEL}" -a "x${VERSION}" != "x" ]; then
	echo "Cannot use latest repository and explicitly specified version." >&2
	exit 1
fi

PROJECT="${HP_OBS_PROJECT}:${REPO}"
#PACKAGE=""
#VERSION=""
RELEASE=""

# Set package to be uploaded.
case "x${PACKAGE}" in
x${P_LIBDATOVKA})
	if [ "x${VERSION}" != "x" ]; then
		# Do nothing.
		echo "" > /dev/null
	else
		# Use latest release as default.
		VERSION="0.6.2"
	fi
	RELEASE="1"
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

# Ask twice when dealing with stable.
if [ "x${ANSWER_YES}" != "xyes" -a "x${REPO}" = "x${HP_REPO_LATEST}" ]; then
	ANSWER=$(ask_user_yn "Do you want to push '${PACKAGE}-${VERSION}-${RELEASE}' into '${PROJECT}'?" "n")
	if [ "x${ANSWER}" != "xy" ]; then
		exit 0
	fi
fi

CMD_OSC="osc"
if [ -z $(command -v "${CMD_OSC}") ]; then
	echo "Install '${CMD_OSC}' to be able to use the openSUSE Build Service." >&2
	exit 1
fi

DISTRO_WORK_DIR="_distrofiles/${PACKAGE}"
dir_present "${DISTRO_WORK_DIR}" || exit 1

PACKAGE_ORIG_SRC="${DISTRO_WORK_DIR}/${PACKAGE}_${VERSION}.orig.tar.xz"
file_present "${PACKAGE_ORIG_SRC}" || exit 1

${CMD_OSC} co "${PROJECT}" "${PACKAGE}"
pushd "${PROJECT}/${PACKAGE}"
${CMD_OSC} del *
cp "${SRC_ROOT}/${PACKAGE_ORIG_SRC}" ./ || exit 1
cp "${SRC_ROOT}/${DISTRO_WORK_DIR}/rpm/${PACKAGE}.spec" ./ || exit 1
if [ "x${PACKAGE}" = "x${P_LIBDATOVKA}" ]; then
	# Do nothing for now.
	echo "" > /dev/null
fi
cp "${SRC_ROOT}/${DISTRO_WORK_DIR}/deb/${PACKAGE}.dsc" ./ || exit 1
cp "${SRC_ROOT}/${DISTRO_WORK_DIR}/deb/${PACKAGE}_${VERSION}-${RELEASE}.debian.tar.xz" ./ || exit 1
if [ "x${ANSWER_YES}" != "xyes" ]; then
	ANSWER=$(ask_user_yn "Do you want to push newly created '${PACKAGE}' package sources to '${PROJECT}' OBS?" "n")
	if [ "x${ANSWER}" != "xy" ]; then
		exit 0
	fi
fi
osc addremove
osc ci -n
popd

echo "OK"
