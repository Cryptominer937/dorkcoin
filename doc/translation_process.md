# Dork Core translations

Dork Core uses Qt translation catalogs for the application and extracted
non-Qt messages. Submit translations and translation fixes to
https://github.com/dork-core/dorkcoin.

The software name is **Dork Core**. The currency, addresses, and network remain
**Dorkcoin**. Keep executable names, configuration paths, URI schemes, format
placeholders, and copyright attribution unchanged when translating prose.

## Updating source strings

Follow the existing `tr()` and `_()` patterns in source. Catalog filenames and
translation contexts retain inherited internal names for compatibility:
`src/qt/locale/bitcoin_<lang>.ts`, `bitcoin_en.xlf`, and `bitcoin-core`.
They are implementation identifiers, not product branding.

With the Qt and gettext tools available, regenerate source catalogs using:

```sh
cd src
make translate
```

Review the generated diff, preserve `%1`, `%n`, `%s`, and other placeholders,
and check plural forms and accelerator keys. Open catalogs with Qt Linguist
to translate or validate them. Rebuild the application and check that messages
remain readable and fit their controls.

## Translation services

No external Dork Core translation service is configured or verified by this
repository. The inherited Transifex project has been disconnected; do not send
new Dorkcoin strings to another project's translation account.

Maintainers may configure a verified project in `.tx/config` and document its
owner, resource, and workflow here before enabling synchronization. Until then,
submit reviewed catalog changes directly through Dork Core pull requests.

## Review and testing

Keep translation changes separate from consensus changes. Check XML validity,
build the Qt resource catalogs, and run relevant Qt tests. Report the tested
language and platform in the pull request. Do not translate Dorkcoin addresses,
keys, hashes, or protocol tokens.
