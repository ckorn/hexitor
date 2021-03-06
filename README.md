hexitor
=======

A terminal hex editor.

![A screenshot](https://s3.amazonaws.com/briansteffens/hexitor.png)

# Installation

If you're using Arch, hexitor is available on the AUR:
<https://aur.archlinux.org/packages/hexitor/>

Otherwise, make sure you have gcc, make, git, and ncurses installed and then:

```bash
git clone https://github.com/briansteffens/hexitor
cd hexitor
make
sudo make install
```

To uninstall:

```bash
sudo make uninstall
```

# Usage

### Opening a file

```bash
hexitor <some_file>
```

### Movement

- Use the arrow keys or *hjkl* to move the cursor around the editor.
- Page up and page down work as expected.
- Type ```:123``` and hit enter to move to the 123rd byte in the file.

### Editing bytes

The keys 0-9 and a-f will overwrite the current nibble (half-byte).

### Saving changes

- Type ```:w``` and hit enter to save the file.
- Type ```:w <some_other_file>``` and hit enter to save changes to a different
  file.

### Quitting

Type ```:q``` and hit enter to quit.
