Import("env")
import os
import shutil

def after_build(source, target, env):
    firmware_path = os.path.join(env.subst("$BUILD_DIR"), "firmware.bin")
    if os.path.exists(firmware_path):
        new_path = os.path.join(env.subst("$PROJECT_DIR"), "build_output", "firmware_v1.bin")
        os.makedirs(os.path.dirname(new_path), exist_ok=True)
        shutil.copy(firmware_path, new_path)
        print(f"✔ Firmware copied to: {new_path}")
    else:
        print("⚠ firmware.bin not found.")

def before_build(source, target, env):
    firmware_path = os.path.join(env.subst("$PROJECT_DIR"), "build_output")
    if os.path.exists(firmware_path):
        shutil.rmtree(firmware_path)
        print(f"✔ Firmware removed from: {firmware_path}")
    else:
        print("⚠ firmware.bin not found.")

# Hook into the post-build step for firmware.bin
env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", after_build)
env.AddPreAction("$BUILD_DIR/${PROGNAME}.bin", before_build)
