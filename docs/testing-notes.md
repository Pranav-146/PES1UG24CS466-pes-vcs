# Testing Notes

The project targets Ubuntu 22.04 with:

```bash
sudo apt update && sudo apt install -y gcc build-essential libssl-dev
```

This checkout was prepared on a Windows host that does not currently expose
`gcc`, `make`, or WSL through the PowerShell session. The expected validation
commands are still included in `Makefile` and `test_sequence.sh`.
