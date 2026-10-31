# Repository administration: protect the main branch

This document records the branch-protection rules currently applied to the `paulomartinspt/lusofw` fork. The ruleset is configured in GitHub repository settings, not in firmware source code.

## Active ruleset

- **Name:** `Protect main`
- **Enforcement:** Active
- **Target:** the default branch, currently `main`
- **Bypass actors:** none; the current administrator cannot bypass the ruleset

The following rules are active:

- Require a pull request before merging.
- Require all review conversations to be resolved before merging.
- Require zero approvals. This is intentional for a single-maintainer fork so the owner can merge their own reviewed PRs.
- Prevent branch deletion.
- Block non-fast-forward updates (force pushes).
- Allow merge, squash and rebase merge methods.

## Settings to review

- **Restrict creations** is currently enabled. It is not needed to protect an existing branch when deletion is already restricted, and may make recovery harder if `main` ever has to be recreated. Consider disabling this rule while keeping deletion protection enabled.

- **Required status checks** are not currently configured. The `Native LusoFW tests` workflow runs four native environments on pull requests and on pushes to `main` and `dev`:
  - `native-tests (native_lusofw)`
  - `native-tests (native_lusofw_pa)`
  - `native-tests (native_lusofw_cap10)`
  - `native-tests (native_identity_store)`

  These checks have passed together on the current fork. Consider making all four required status checks in the `Protect main` ruleset so the pull-request requirement is backed by enforced test results. They cover native feature tests, not board-specific firmware builds or hardware validation.

- **Approvals** can be raised to one or more if another trusted maintainer is added to the repository. With the current single-maintainer setup, zero approvals avoids blocking the maintainer's own PRs.

## Toolchain note

The repository's CI and release workflows install PlatformIO Core from `requirements-platformio.txt` (`platformio==6.2.0`). Library and platform/framework dependency ranges are not yet fully locked, so this is an incremental reproducibility improvement, not a complete dependency lock.

## Where to configure

Open **Settings → Rules → Rulesets** and edit `Protect main`. To require checks, enable **Require status checks to pass** and select the four job names listed above. Repository ruleset changes are administrative settings and cannot be applied by a source-code pull request.
