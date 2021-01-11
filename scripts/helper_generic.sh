#!/usr/bin/env sh

# Ask question and waint for user response.
ask_user_yn () {
	local QUESTION="$1"
	local DFLT="$2" # May be 'y' on 'n'.

	local Y="y"
	local N="n"

	# Set default value.
	case "${DFLT}" in
	[Yy]*)
		Y="Y"
		DFLT="y"
		;;
	[Nn]*)
		N="N"
		DFLT="n"
		;;
	*)
		DFLT=""
		;;
	esac

	local ANSWER=""
	while true; do
		read -p "${QUESTION} (${Y}/${N}) " ANSWER
		case "x${ANSWER}" in
		x[Yy]*)
			echo "y"
			return 0
			;;
		x[Nn]*)
			echo "n"
			return 0
			;;
		x)
			if [ "x${DFLT}" != "x" ]; then
				echo "${DFLT}"
				return 0
			fi
			echo "Please answer yes or no." >&2
			;;
		*)
			echo "Please answer yes or no." >&2
			;;
		esac
	done
}
