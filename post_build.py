
Import("env")
import os
import shutil

def copy_firmware(source, target, env):
    firmware_path = target[0].get_abspath()
    new_path = os.path.join(env.subst("$PROJECT_DIR"), "build_output", "my_firmware_v1.bin")
    os.makedirs(os.path.dirname(new_path), exist_ok=True)
    shutil.copy(firmware_path, new_path)
    print(f"✔ Firmware copied to: {new_path}")

# Hook into the post-build step for firmware.bin
env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", copy_firmware)
