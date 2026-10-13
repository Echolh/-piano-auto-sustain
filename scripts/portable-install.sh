#!/usr/bin/env bash
set -euo pipefail
package="$(cd "$(dirname "$0")" && pwd)"
action="$(basename "$0")"
case "$(uname -s)" in
  Darwin) platform=macOS; parent="$HOME/Library/Audio/Plug-Ins";;
  Linux) platform=Linux; parent="$HOME/.vst3";;
  *) echo 'This package is for macOS or Linux.'; exit 1;;
esac
grep -q "\"platform\": \"$platform-" "$package/BUILD-INFO.json" || { echo 'Wrong operating-system package.'; exit 1; }
if [[ "$action" != Uninstall.command ]]; then
  (cd "$package"; if [[ "$platform" == macOS ]]; then shasum -a 256 -c SHA256SUMS.txt; else sha256sum -c SHA256SUMS.txt; fi)
fi
for format in VST3 AU; do
  if [[ "$format" == AU ]]; then
    [[ "$platform" == macOS ]] || continue
    name='Piano Auto Sustain MIDI FX.component'; destination="$parent/Components"
  else
    name='Piano Auto Sustain.vst3'
    if [[ "$platform" == macOS ]]; then destination="$parent/VST3"; else destination="$parent"; fi
  fi
  target="$destination/$name"; receipt="$destination/.PianoAutoSustain-$format-installed"
  [[ ! -L "$target" && ! -L "$receipt" ]] || { echo 'Refusing symbolic-link target.'; exit 1; }
  if [[ "$action" == Uninstall.command ]]; then
    if [[ -f "$receipt" ]]; then
      [[ "$(cat "$receipt")" == "$name" ]] || { echo 'Unknown ownership receipt.'; exit 1; }
      rm -rf -- "$target"; rm -f -- "$receipt"
      echo "Removed $target"
    else echo "No installer-owned $format installation found."; fi
  else
    [[ -d "$package/$name" ]] || { echo 'Bundle missing.'; exit 1; }
    mkdir -p "$destination"
    if [[ -e "$target" && ! -f "$receipt" ]]; then echo "Existing manual installation: back up and remove $target first."; exit 1; fi
    staging="$(mktemp -d "$destination/.PianoAutoSustain-stage.XXXXXX")"
    trap 'rm -rf -- "$staging"' EXIT
    cp -R "$package/$name" "$staging/$name"
    if [[ "$platform" == macOS ]]; then codesign --verify --deep --strict "$staging/$name"; fi
    backup="$staging/previous"
    if [[ -e "$target" ]]; then mv "$target" "$backup"; fi
    if ! mv "$staging/$name" "$target"; then
      if [[ -e "$backup" ]]; then mv "$backup" "$target"; fi
      exit 1
    fi
    printf '%s' "$name" > "$receipt"
    rm -rf -- "$staging"; trap - EXIT
    echo "Installed $target"
  fi
done
echo 'Done. Restart your DAW and rescan plugins. / 完成，请重启宿主并扫描插件。'
