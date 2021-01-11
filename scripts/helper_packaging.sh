#!/usr/bin/env sh

# Global variables defined here use the 'HP_' prefix.
HP_OBS_PROJECT="home:CZ-NIC"
HP_REPO_DEVEL="datovka-devel"
HP_REPO_LATEST="datovka-latest"

# Check directory presence.
dir_present () {
	local DIR="$1"

	if [ "x${DIR}" = "x" ]; then
		echo "Missing directory name as parameter." >&2
		return 1
	fi

	if [ ! -d "${DIR}" ]; then
		echo "Cannot find directory '${DIR}'." >&2
		return 1
	fi

	return 0
}

# Check file presence.
file_present () {
	local FILE="$1"

	if [ "x${FILE}" = "x" ]; then
		echo "Missing file name as parameter." >&2
		return 1
	fi

	if [ ! -f "${FILE}" ]; then
		echo "Cannot find file '${FILE}'." >&2
		return 1
	fi

	return 0
}

# Delete and recreate empty directory.
rm_and_create_dir () {
	local DIR="$1"

	if [ "x${DIR}" = "x" ]; then
		echo "Missing directory name as parameter." >&2
		return 1
	fi

	rm -rf "${DIR}"
	mkdir -p "${DIR}"
	if [ 0 != $? ]; then
		return 1
	fi

	return 0
}

# Remove *.swp files.
rm_swp_files () {
	local DIR="$1"

	if [ "x${DIR}" = "x" ]; then
		echo "Missing directory name as parameter." >&2
		return 1
	fi

	if [ ! -d "${DIR}" ]; then
		echo "'${DIR}' is not a directory." >&2
		return 1
	fi
	for f in $(find "${DIR}/"); do
		if [ -f "${f}" ]; then
			SWP_FILE=$(echo "${f}" | grep '[.]swp$')
			if [ "x${SWP_FILE}" != "x" ]; then
				echo "Removing '${SWP_FILE}'."
				rm "${SWP_FILE}"
			fi
		fi
	done
	return 0
}

# Fill VERSION and RELEASE fields in distribution specific files.
fill_in_files () {
	local DIR="$1"

	if [ "x${DIR}" = "x" ]; then
		echo "Missing directory name as parameter." >&2
		return 1
	fi

	local SUBST_FILES=$(find "${DIR}" | grep '[.]in$')
	for f in ${SUBST_FILES}; do
		OUT_NAME=$(echo "${f}" | sed 's/[.]in$//g')
		echo "'${f}' -> '${OUT_NAME}'"
		cat "${f}" | sed -e "s/__VERSION__/${VERSION}/g" -e "s/__RELEASE__/${RELEASE}/g" > "${OUT_NAME}"

		# Preserve timestamps.
		touch -r "${f}" "${OUT_NAME}"

		rm "${f}"
	done

	return 0
}

# Compute MD5 checksum.
compute_md5_checksum () {
	local FILE_NAME="$1"

	if [ "x${FILE_NAME}" = "x" -o ! -f "${FILE_NAME}" ]; then
		echo ""
		echo "File name is empty or '${FILE_NAME}' does not exist." >&2
		return 1
	fi

	local CMD_SHA256SUM=md5sum
	local CMD_OPENSSL=openssl

	if [ -z $(command -v "${CMD_OPENSSL}") ]; then
		echo "Install '${CMD_OPENSSL}' to be able to check checksum file." >&2
		CMD_OPENSSL=""
	fi

	if [ "x${CMD_OPENSSL}" = "x" ]; then
		echo ""
		return 1
	fi

	"${CMD_OPENSSL}" md5 "${FILE_NAME}" | sed -e 's/^.*\s//g'
	if [ 0 != $? ]; then
		return 1
	fi

	return 0
}
