# My Own Wii U CFW
**An ARM firmware image for the Wii U, designed to to be my own custom cfw for the wii u**

### Getting The Custom Wii U CFW
##### Prebuilt Download
A precompiled fw.img is not available [github.com/PokeyManatee4/actions]. Don't think about it too much. Once you've got that, jump down to "Setup" and keep reading.

##### Compiling
First, edit `castify.py` to add the Starbuck key/IV - this is required to work with current CFW loaders. Then, just run make. You'll need devkitARM installed. You should end up with a fw.img, along with a bunch of ELF files.

You should now find a fw.img in your cfw folder! Congrats. Read the "Setup" section next!

### Setup
You'll need an SD card normally readable by the Wii U. Simply copy the fw.img to the root of the SD; then just run it.

### Running
Once you've done everything in the Setup stage, just run your favorite CFW booter (CBHC works, as does the old wiiubru loader, and the Aroma beta). With any luck, you should see Hello World.
