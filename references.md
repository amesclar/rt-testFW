# References

## cli board upload

```bash

arduino-cli compile --upload -p /dev/ttyACM0 --fqbn arduino:avr:uno .
```

## dialout group (Linux)

To fix "Permission Denied" errors on serial ports:

```bash
sudo usermod -aG dialout $USER
```

*Note: You must log out and back in for this to take effect.*

### Immediate effect (current terminal only)

```bash
newgrp dialout
```
