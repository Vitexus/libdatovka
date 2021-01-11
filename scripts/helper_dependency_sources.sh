#!/usr/bin/env sh

# Compute SHA256 checksum.
check_sha256 () {
	local FILE_NAME="$1"
	local SHA256_HASH="$2"

	local CMD_OPENSSL=openssl
	local CMD_SED=sed
	if [ $(uname) = "Darwin" ]; then
		# OS X version of sed does not recognise \s as white space
		# identifier.
		CMD_SED=gsed
	fi

	if [ "x${FILE_NAME}" = "x" ]; then
		echo "No file name supplied." >&2
		return 1
	fi
	if [ "x${SHA256_HASH}" = "x" ]; then
		echo "No sha256 checksum for '${FILE_NAME}' supplied." >&2
		return 1
	fi

	if [ -z $(command -v "${CMD_OPENSSL}") ]; then
		echo "Install '${CMD_OPENSSL}' to be able to compute file checksum." >&2
		return 1
	fi
	if [ -z $(command -v "${CMD_SED}") ]; then
		echo "Install '${CMD_SED}' to be able to compute file checksum." >&2
		return 1
	fi

	COMPUTED_SHA256=$("${CMD_OPENSSL}" sha256 "${FILE_NAME}" | "${CMD_SED}" -e 's/^.*\s//g')

	if [ "x${COMPUTED_SHA256}" != "x${SHA256_HASH}" ]; then
		echo "'${FILE_NAME}' sha256 checksum mismatch." >&2
		echo "'${FILE_NAME}' computed sha256: '${COMPUTED_SHA256}'" >&2
		echo "'${FILE_NAME}' expected sha256: '${SHA256_HASH}'" >&2
		return 1
	fi

	echo "'${FILE_NAME}' sha256 checksum OK."
	return 0
}

# Check signature.
check_sig () {
	local FILE_NAME="$1"
	local SIG_FILE_NAME="$2"
	local KEY_FP="$3"

	local CMD_GPG=gpg

	if [ "x${FILE_NAME}" = "x" ]; then
		echo "No file name supplied." >&2
		return 1
	fi
	if [ "x${SIG_FILE_NAME}" = "x" ]; then
		echo "No signature file name supplied." >&2
		return 1
	fi
	if [ "x${KEY_FP}" = "x" ]; then
		echo "Missing key fingerprint to signature of '${ARCHIVE_FILE}'." >&2
		return 1
	fi

	if [ -z $(command -v "${CMD_GPG}") ]; then
		echo "Install '${CMD_GPG}' to be able to compute file checksum." >&2
		return 1
	fi

	local GPG_DIR="./.gnupg_work"
	rm -rf "${GPG_DIR}"
	mkdir -p "${GPG_DIR}"

	local GPG_PARAMS="--homedir ${GPG_DIR} --no-default-keyring --keyring keys.gpg --no-permission-warning"
	#local KEY_SRVR="sks.labs.nic.cz"
	#local KEY_SRVR="pool.sks-keyservers.net"
	#local KEY_SRVR="pgp.mit.edu"
	local KEY_SRVR="keys.gnupg.net"
	"${CMD_GPG}" ${GPG_PARAMS} --keyserver "${KEY_SRVR}" --recv "${KEY_FP}"
	"${CMD_GPG}" ${GPG_PARAMS} --verify "${SIG_FILE_NAME}" "${FILE_NAME}"
	local GPG_RET="$?"
	rm -rf "${GPG_DIR}"
	if [ "${GPG_RET}" != "0" ]; then
		echo "'${FILE_NAME}' signature check failed." >&2
		return 1
	fi

	echo "'${FILE_NAME}' signature OK."
	return 0
}

# Download file.
download_file () {
	local ARCHIVE_FILE="$1"
	local URL_PREFIX="$2"

	local CMD_CURL=curl

	if [ "x${ARCHIVE_FILE}" = "x" ]; then
		echo "Missing archive file name." >&2
		return 1
	fi
	if [ "x${URL_PREFIX}" = "x" ]; then
		echo "Missing URL prefix for archive '${ARCHIVE_FILE}'." >&2
		return 1
	fi

	if [ -z $(command -v "${CMD_CURL}") ]; then
		echo ""
		echo "Install '${CMD_CURL}' to be able to compute file checksum." >&2
		return 1
	fi

	"${CMD_CURL}" -L -O "${URL_PREFIX}${ARCHIVE_FILE}"
}

# Download sources.
ensure_source_presence () {
	local SRC_DIR="$1"
	local ARCHIVE_FILE="$2"
	local URL_PREFIX="$3"
	local SHA256_HASH="$4"
	local SIG_SUF="$5"
	local KEY_FP="$6"

	local ORIG_DIR=$(pwd)
	local ARCHIVE_SIG_FILE=""

	if [ "x${SRC_DIR}" = "x" -o ! -d "${SRC_DIR}" ]; then
		echo "'${SRC_DIR}' is not a directory." >&2
		return 1
	fi
	if [ "x${ARCHIVE_FILE}" = "x" -o -e "${SRC_DIR}/${ARCHIVE_FILE}" -a ! -f "${SRC_DIR}/${ARCHIVE_FILE}" ]; then
		echo "Missing archive file '${ARCHIVE_FILE}' argument or '${SRC_DIR}/${ARCHIVE_FILE}' is not a file." >&2
		return 1
	fi
	if [ "x${URL_PREFIX}" = "x" ]; then
		echo "Missing URL prefix for archive '${ARCHIVE_FILE}'." >&2
		return 1
	fi
	if [ "x${SHA256_HASH}" = "x" -a "x${SIG_SUF}" = "x" ]; then
		echo "Cannot check '${ARCHIVE_FILE}' because no hash or signature is supplied." >&2
		return 1
	fi
	if [ "x${SIG_SUF}" != "x" ]; then
		ARCHIVE_SIG_FILE="${ARCHIVE_FILE}${SIG_SUF}"
		if [ -e "${SRC_DIR}/${ARCHIVE_SIG_FILE}" -a ! -f "${SRC_DIR}/${ARCHIVE_SIG_FILE}" ]; then
			echo "'${SRC_DIR}/${ARCHIVE_SIG_FILE}' is not a file." >&2
			return 1
		fi
		if [ "x${KEY_FP}" = "x" ]; then
			echo "Missing key fingerprint to signature of '${ARCHIVE_FILE}'." >&2
			return 1
		fi
	fi

	cd "${SRC_DIR}"

	# Download signature file.
	if [ "x${ARCHIVE_SIG_FILE}" != "x" -a ! -e "${ARCHIVE_SIG_FILE}" ]; then
		if ! download_file "${ARCHIVE_SIG_FILE}" "${URL_PREFIX}"; then
			echo "Cannot download '${ARCHIVE_SIG_FILE}'." >&2
			return 1
		fi
		if [ ! -f "${ARCHIVE_SIG_FILE}" ]; then
			echo "'${ARCHIVE_SIG_FILE}' is missing." >&2
			return 1
		fi
	fi

	if [ -e "${ARCHIVE_FILE}" ]; then
		CHECK_SHA256_RET="0"
		if [ "x${SHA256_HASH}" != "x" ]; then
			check_sha256 "${ARCHIVE_FILE}" "${SHA256_HASH}"
			CHECK_SHA256_RET="$?"
		fi

		if [ "${CHECK_SHA256_RET}" = "0" ]; then
			if [ "x${ARCHIVE_SIG_FILE}" != "x" ]; then
				if check_sig "${ARCHIVE_FILE}" "${ARCHIVE_SIG_FILE}" "${KEY_FP}"; then
					cd "${ORIG_DIR}"
					return 0
				fi
			else
				cd "${ORIG_DIR}"
				return 0
			fi
		fi

		# Checksum mismatch.
		echo "Trying to download '${ARCHIVE_FILE}'." >&2
		mv "${ARCHIVE_FILE}" "bad_${ARCHIVE_FILE}"
		if [ "x${ARCHIVE_SIG_FILE}" != "x" ]; then
			echo "Trying to download '${ARCHIVE_SIG_FILE}'." >&2
			mv "${ARCHIVE_SIG_FILE}" "bad_${ARCHIVE_SIG_FILE}"
		fi
	fi

	# Download source file.
	if [ ! -e "${ARCHIVE_FILE}" ]; then
		if ! download_file "${ARCHIVE_FILE}" "${URL_PREFIX}"; then
			echo "Cannot download '${ARCHIVE_FILE}'." >&2
			return 1
		fi
	fi
	if [ ! -f "${ARCHIVE_FILE}" ]; then
		echo "'${ARCHIVE_FILE}' is missing." >&2
		return 1
	fi

	# Download signature file.
	if [ "x${ARCHIVE_SIG_FILE}" != "x" -a ! -e "${ARCHIVE_SIG_FILE}" ]; then
		if ! download_file "${ARCHIVE_SIG_FILE}" "${URL_PREFIX}"; then
			echo "Cannot download '${ARCHIVE_SIG_FILE}'." >&2
			return 1
		fi
		if [ ! -f "${ARCHIVE_SIG_FILE}" ]; then
			echo "'${ARCHIVE_SIG_FILE}' is missing." >&2
			return 1
		fi
	fi

	if [ "x${SHA256_HASH}" != "x" ]; then
		check_sha256 "${ARCHIVE_FILE}" "${SHA256_HASH}" || return 1
	fi
	if [ "x${ARCHIVE_SIG_FILE}" != "x" ]; then
		check_sig "${ARCHIVE_FILE}" "${ARCHIVE_SIG_FILE}" "${KEY_FP}" || return 1
	fi

	cd "${ORIG_DIR}"
	return 0
}

# Decompress compressed archive.
decompress_archive () {
	ARCHIVE="$1"
	if [ "x${ARCHIVE}" = "x" ]; then
		echo "Use parameter for '$0'" >&2
		exit 1
	fi

	DECOMPRESS_CMD=""
	case "${ARCHIVE}" in
	*.tar.gz)
		DECOMPRESS_CMD="tar -xzf"
		;;
	*.tar.bz2)
		DECOMPRESS_CMD="tar -xjf"
		;;
	*.tar.xz)
		DECOMPRESS_CMD="tar -xJf"
		;;
	*)
		;;
	esac

	if [ "x${DECOMPRESS_CMD}" = "x" ]; then
		echo "Don't know how to decompress '${ARCHIVE}'." >&2
		exit 1
	fi

	${DECOMPRESS_CMD} "${ARCHIVE}" || exit 1
}

# First erase any potential targets, then decompress.
erase_and_decompress () {
	SRC_DIR="$1"
	ARCHIVE_FILE="$2"
	WORK_DIR="$3"
	ARCHIVE_NAME="$4"

	if [ "x${SRC_DIR}" = "x" ]; then
		echo "No source directory specified." >&2
		exit 1
	fi
	if [ "x${ARCHIVE_FILE}" = "x" ]; then
		echo "No archive file specified." >&2
		exit 1
	fi
	if [ "x${WORK_DIR}" = "x" ]; then
		echo "No working directory specified." >&2
		exit 1
	fi
	if [ "x${ARCHIVE_NAME}" = "x" ]; then
		echo "No archive name specified." >&2
		exit 1
	fi

	ARCHIVE="${SRC_DIR}/${ARCHIVE_FILE}"
	if [ ! -f "${ARCHIVE}" ]; then
		echo "Missing file '${ARCHIVE}'." >&2
		exit 1
	fi
	rm -rf "${WORK_DIR}"/"${ARCHIVE_NAME}"*
	cd "${WORK_DIR}"
	decompress_archive "${ARCHIVE}"
}
