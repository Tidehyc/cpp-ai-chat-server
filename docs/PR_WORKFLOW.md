# PR Workflow

This repository now has a repeatable local workflow for test and PR creation.

## GitHub CLI

- GitHub CLI is installed locally under `~/.local/bin/gh`.
- Because GitHub traffic is intercepted by a local TLS proxy, the CLI needs a CA bundle that includes the `SteamTools` root certificate.
- The generated bundle lives at `~/.local/gh-certs/ca-bundle.pem`.

## Login

Use this command to authenticate:

```bash
SSL_CERT_FILE=$HOME/.local/gh-certs/ca-bundle.pem $HOME/.local/bin/gh auth login -h github.com -w --git-protocol https
```

## Suggested PR Flow

1. Run the relevant tests.
2. Commit the change on a feature branch.
3. Create the PR with `gh pr create`.
4. Merge after the required checks pass.

## Notes

- If `gh auth login` fails, check that the `SteamTools` CA bundle still exists and is readable.
- If the environment changes, regenerate the bundle from the NSS database in `~/.pki/nssdb`.
