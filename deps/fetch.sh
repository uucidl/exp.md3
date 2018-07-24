#!/usr/bin/env bash
set -e
depfile="${1:?missing depfile}"
depname="$(basename "${depfile}" .txt)"
url="$(sed -n -e 's/@url: *//p' "${depfile}")"
name="$(sed -n -e 's/@name: *//p' "${depfile}")"
if [[ -z $"{name}" ]] ; then name="$(basename "${depfile}" .txt)" ; fi
package="${depname}.zip"
curl -L "${url}" -o "${package}"
depdir="./"${depname}""
[[ -d "${depdir}" ]] && rm -rf "${depdir}"
unzip "${package}" -d "${depdir}"
mv "${depdir}"/"${name}"-*/* "${depdir}"
rmdir "${depdir}"/"${name}"-*

