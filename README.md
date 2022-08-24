# My Own Wii U CFW
**An ARM firmware image for the Wii U, designed to to be my own custom cfw for the wii u**

### Getting THe Custom Wii U CFW
##### Prebuilt Download
A precompiled fw.img is not available [here](bonk.com). Don't think about it too much. Once you've got that, jump down to "Setup" and keep reading.

##### Compiling (Docker)
The preferred method of compiling is using our Docker image. Clone or download this repository, then:
```sh
#replace with wherever you cloned this repo
cd path/to/linux-loader
#Replace with the Starbuck key and IV, respectively. Make sure it's in all caps
sed s/B5XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX/STARBUCK_KEY_HERE/g -i castify.py
sed s/91XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX/STARBUCK_IV_HERE/g -i castify.py
#Run the image! (also works in Podman)
docker run --rm -it -v $(pwd):/app registry.gitlab.com/linux-wiiu/linux-loader/ci
```
You should now find a fw.img in your linux-loader folder! Congrats. Read the "Setup" section next!

##### Compiling (from scratch, not recommended)
First, edit `castify.py` to add the Starbuck key/IV - this is required to work with current CFW loaders. Then, just run make. You'll need devkitARM installed. You should end up with a fw.img, along with a bunch of ELF files.

### Setup
You'll need an SD card normally readable by the Wii U. Simply copy the fw.img to the root of the SD; then copy `dtbImage.wiiu` to `sd:/linux` (you should end up with `sd:/linux/dtbImage.wiiu`).

### Running
Once you've done everything in the Setup stage, just run your favorite CFW booter (CBHC works, as does the old wiiubru loader, and the Aroma beta). With any luck, you should see Hello World.
