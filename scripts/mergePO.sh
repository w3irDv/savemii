export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH

# Run me from the repo root: scripts/mergePO.sh

find . | grep -E "src.*\.cpp$|include.*\.h$"  | xargs xgettext --escape  --no-wrap --keyword=_  --language=C++ --from-code=UTF-8  --output=pot/messages.pot -

msgmerge --no-wrap --update romfs/locales/english.po pot/messages.pot
msgmerge --no-wrap --update romfs/locales/spanish.po pot/messages.pot
msgmerge --no-wrap --update romfs/locales/SChinese.po pot/messages.pot
msgmerge --no-wrap --update romfs/locales/german.po pot/messages.pot
msgmerge --no-wrap --update romfs/locales/portuguese.po pot/messages.pot
msgmerge --no-wrap --update romfs/locales/italian.po pot/messages.pot
msgmerge --no-wrap --update romfs/locales/japanese.po pot/messages.pot
msgmerge --no-wrap --update romfs/locales/korean.po pot/messages.pot
msgmerge --no-wrap --update romfs/locales/russian.po pot/messages.pot
msgmerge --no-wrap --update romfs/locales/TChinese.po pot/messages.pot
