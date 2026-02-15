export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH

# Run me from the repo root: scripts/mergePO.sh

find . | grep -E "src.*\.cpp$|include.*\.h$"  | xargs xgettext --escape  --no-wrap --keyword=_  --language=C++ --from-code=UTF-8  --output=romfs/messages.pot -

msgmerge --no-wrap --update romfs/locales/english.po romfs/messages.pot
msgmerge --no-wrap --update romfs/locales/spanish.po romfs/messages.pot
msgmerge --no-wrap --update romfs/locales/SChinese.po romfs/messages.pot
msgmerge --no-wrap --update romfs/locales/german.po romfs/messages.pot
msgmerge --no-wrap --update romfs/locales/portuguese.po romfs/messages.pot
msgmerge --no-wrap --update romfs/locales/italian.po romfs/messages.pot
msgmerge --no-wrap --update romfs/locales/japanese.po romfs/messages.pot
msgmerge --no-wrap --update romfs/locales/korean.po romfs/messages.pot
msgmerge --no-wrap --update romfs/locales/russian.po romfs/messages.pot
msgmerge --no-wrap --update romfs/locales/TChinese.po romfs/messages.pot
