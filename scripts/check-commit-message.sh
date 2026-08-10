#!/usr/bin/env bash
# Enforce the commit message layout used in this repository:
#   * subject line at most 52 characters
#   * blank line between subject and body
#   * body wrapped at 72 characters
#
# Trailers (Signed-off-by:, Co-Authored-By: ...) and unbreakable tokens such as
# URLs are exempt from the body limit. Merge, revert, fixup and squash messages
# are left alone because git writes them itself.
set -e

readonly MAX_SUBJECT=52
readonly MAX_BODY=72

msg_file="${1:-}"
if [ -z "${msg_file}" ] || [ ! -f "${msg_file}" ]; then
  echo "usage: $(basename "$0") <commit-msg-file>" >&2
  exit 2
fi

lines=()
while IFS= read -r line || [ -n "${line}" ]; do
  case "${line}" in
    '#'*) continue ;;
  esac
  lines+=("${line}")
done <"${msg_file}"

# Drop the leading blank lines git may have left behind.
while [ "${#lines[@]}" -gt 0 ] && [ -z "${lines[0]}" ]; do
  lines=("${lines[@]:1}")
done

if [ "${#lines[@]}" -eq 0 ]; then
  echo "commit message is empty" >&2
  exit 1
fi

subject="${lines[0]}"
case "${subject}" in
  'Merge '* | 'Revert '* | 'fixup!'* | 'squash!'*) exit 0 ;;
esac

status=0

if [ "${#subject}" -gt "${MAX_SUBJECT}" ]; then
  echo "subject is ${#subject} characters, the limit is ${MAX_SUBJECT}:" >&2
  echo "  ${subject}" >&2
  status=1
fi

if [ "${#lines[@]}" -gt 1 ] && [ -n "${lines[1]}" ]; then
  echo "the subject must be followed by a blank line" >&2
  status=1
fi

for ((i = 2; i < ${#lines[@]}; i++)); do
  body_line="${lines[i]}"
  # A trailer, or a single unbreakable token such as a URL.
  if [[ "${body_line}" =~ ^[A-Za-z][A-Za-z-]*:[[:space:]] ]] || [[ "${body_line}" != *" "* ]]; then
    continue
  fi
  if [ "${#body_line}" -gt "${MAX_BODY}" ]; then
    echo "body line $((i + 1)) is ${#body_line} characters, the limit is ${MAX_BODY}:" >&2
    echo "  ${body_line}" >&2
    status=1
  fi
done

exit "${status}"
