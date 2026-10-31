# Repository administration: protect the main branch

This is a repository-settings recommendation, not a firmware source change.

## Recommended branch rules

Apply a ruleset to the `main` branch with the following settings:

- Require a pull request before merging.
- Require at least one approval.
- Require all review conversations to be resolved.
- Require the native LusoFW tests workflow to pass.
- Block force pushes.
- Block branch deletion.
- Require the branch to be up to date before merging once the required CI checks are stable.

## Suggested rollout

1. Enable pull requests, approvals, and protection against force-push/deletion first.
2. Add the native test workflow as a required check after confirming it reports reliably.
3. Add selected firmware build-matrix checks after their duration and target coverage are understood.
4. Review administrator bypass settings and keep them as restrictive as practical.

## Where to configure

Open the repository's **Settings** and configure a branch ruleset for `main`. The available controls may differ depending on the repository plan and GitHub UI.

A code pull request cannot itself enable these settings; an owner must apply them in repository settings.
