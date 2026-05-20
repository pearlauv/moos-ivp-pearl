# Agent Notes

- Before broad repo exploration, consult `docs/context/repo_map.md` when present; regenerate it with `python3 scripts/generate_context_map.py` after changing source metadata that feeds the map, such as `src/CMakeLists.txt`, `src/*/CMakeLists.txt`, mission `.moos`/`.bhv` files, launch scripts, or mission README files.
- Check generated context-map freshness with `python3 scripts/generate_context_map.py --check`.
- Use Conventional Commits for commit messages when committing from this repo: https://www.conventionalcommits.org/en/v1.0.0/
