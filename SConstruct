import os
import time

class term_format:
	default = "\x1b[0m"
	bold = "\x1b[1m"
	black = "\x1b[30m"
	red = "\x1b[31m"
	green = "\x1b[32m"
	yellow = "\x1b[33m"
	blue = "\x1b[34m"
	magenta = "\x1b[35m"
	cyan = "\x1b[36m"
	white = "\x1b[37m"

def cformat(s):
	return s.format(c = term_format)

def make_ver(target, source, env):
	for t in target:
		with open(str(t), "w") as f:
			f.write("#define UBLOAD_VERSION \"%s, %s, %s\"\n" % (env["GIT_BRANCH"], env["GIT_REV"], env["GIT_DATE"]))
			f.write("#define UBLOAD_BUILD_DATE \"%s\"\n" % env["BUILD_DATE"])

	return None


# Create default environment and export it. It will be modified later
# by platform-specific build scripts.
env = Environment()
Export("env")

# Get build platform from the PLATFORM command line argument. If none is
# specified, build for qnode4 platform.
env["PLATFORM"] = ARGUMENTS.get("PLATFORM", "qnode4")

# Get PATH directories from os environment
env.Append(ENV = {"PATH": os.environ["PATH"]})

# get some statistics from git repo about currently built sources
# TODO: provide some defaults if this is not a git repository
env["GIT_BRANCH"] = os.popen("git rev-parse --abbrev-ref HEAD").read().rstrip()
env["GIT_REV"] = os.popen("git rev-parse --short HEAD").read().rstrip()
env["GIT_DATE"] = os.popen("git log -1 --format=%ci").read().rstrip()

# create build version information
#~ env["BUILD_DATE"] = time.ctime(time.time())
env["BUILD_DATE"] = time.strftime("%F %T %Z", time.localtime())

if ARGUMENTS.get('VERBOSE') != "1":
	env['CCCOMSTR'] = cformat("Compiling {c.green}$TARGET{c.default}")
	env['LINKCOMSTR'] = cformat("Linking {c.bold}{c.green}$TARGET{c.default}")
	env['ARCOMSTR'] = cformat("{c.green}{c.bold}Creating library $TARGET{c.default}")

objs = []

# Run platform sconscript to fill some missing parts of the build environment
objs.append(SConscript("platforms/%s/SConscript" % env["PLATFORM"]))

env["CC"] = "%s-gcc" % env["TOOLCHAIN"]
env["CXX"] = "%s-g++" % env["TOOLCHAIN"]
env["AR"] = "%s-ar" % env["TOOLCHAIN"]
env["AS"] = "%s-as" % env["TOOLCHAIN"]
env["LD"] = "%s-gcc" % env["TOOLCHAIN"]
env["OBJCOPY"] = "%s-objcopy" % env["TOOLCHAIN"]
env["OBJDUMP"] = "%s-objdump" % env["TOOLCHAIN"]
env["SIZE"] = "%s-size" % env["TOOLCHAIN"]
env["OOCD"] = "openocd"

# Add platform specific things
env.Append(CPPPATH = [Dir("platforms/%s" % env["PLATFORM"])])

# Add opencm3 library
env.Append(CPPPATH = [Dir("libopencm3/include")])
env.Append(LIBPATH = [Dir("libopencm3/lib")])

# Add common parts of ubload
objs.append(env.Object(source = [File(Glob("common/*.c"))]))
env.Append(CPPPATH = [Dir("common")])

# Add lineedit library
objs.append(env.Object(source = [File(Glob("lineedit/*.c"))]))
env.Append(CPPPATH = [Dir("lineedit")])

# Add crypto library
objs.append(env.Object(source = [File(Glob("crypto/*.c"))]))
env.Append(CPPPATH = [Dir("crypto")])

# Add qdl library
objs.append(env.Object(source = [File(Glob("qdl/*.c"))]))
env.Append(CPPPATH = [Dir("qdl")])

# Add FreeRTOS (port specific sources must be added in the platform SConscript)
objs.append(env.Object(source = [File(Glob("freertos/*.c"))]))
objs.append(env.Object(source = [File(Glob("freertos/portable/%s/*.c" % env["FREERTOS_PORT"]))]))
objs.append(env.Object(source = [File("freertos/portable/MemMang/heap_3.c")]))
env.Append(CPPPATH = [Dir("freertos/include")])
env.Append(CPPPATH = [Dir("freertos/portable/%s" % env["FREERTOS_PORT"])])

# Add uMeshFw HAL
env.Append(CPPPATH = [
	Dir("hal/common"),
	Dir("hal/modules"),
	Dir("hal/interfaces"),
])
objs.append(env.Object(source = [
	File(Glob("hal/common/*.c")),
	File(Glob("hal/modules/*.c")),
	File(Glob("hal/interfaces/*.c")),
]))

env.Append(LINKFLAGS = [
	env["CFLAGS"],
	"--static",
	"-nostartfiles",
	"--specs=nano.specs",
	"-T", env["LDSCRIPT"],
	"-Wl,-Map=ubload_%s.map" % env["PLATFORM"],
	"-Wl,--gc-sections",
])

env.Append(CFLAGS = [
	"-Os",
	"-g",
	"-flto",
	"-fno-common",
	"-ffunction-sections",
	"-fdata-sections",
	"--std=c99",
	"-Wall",
	"-Wextra",
	"-pedantic",
	#~ "-Werror",
	"-Winit-self",
	"-Wreturn-local-addr",
	"-Wswitch-default",
	"-Wuninitialized",
	"-Wundef",
	"-Wstack-usage=256",
	"-Wshadow",
	"-Wimplicit-function-declaration",
	"-Wcast-qual",
	#~ "-Wwrite-strings",
	#~ "-Wconversion",
	"-Wlogical-op",
	"-Wmissing-declarations",
	"-Wno-missing-field-initializers",
	"-Wstack-protector",
	"-Wredundant-decls",
	"-Wmissing-prototypes",
	"-Wstrict-prototypes",
])



# show banner and build configuration
print cformat("{c.bold}{c.blue}uBLoad branch %s, revision %s, date %s{c.default}\n" % (env["GIT_BRANCH"], env["GIT_REV"], env["GIT_DATE"]))
print cformat("{c.bold}Build configuration:{c.default}")
print cformat("\tbuild date = %s" % env["BUILD_DATE"])
print cformat("\tplatform = %s" % env["PLATFORM"])
print cformat("\ttoolchain = %s" % env["TOOLCHAIN"])
print ""

# link the whole thing
elf = env.Program(source = objs, target = "ubload_%s.elf" % env["PLATFORM"], LIBS = [env["LIBOCM3"], "c", "gcc", "nosys", "m"])

env.Append(BUILDERS = {"MakeVer": env.Builder(action = make_ver)})
version = env.MakeVer(target = "platforms/%s/version.h" % env["PLATFORM"], source = None)
Depends(elf, version)

# convert to raw binary
rawbin = env.Command("ubload_%s.bin" % env["PLATFORM"], elf, "$OBJCOPY -O binary $SOURCE $TARGET")

elfsize = env.Command(source = elf, target = "elfsize", action = "$SIZE $SOURCE")

program = env.Command(source = elf, target = "program", action = """
	$OOCD \
	-f interface/%s.cfg \
	-f target/%s.cfg \
	-c "init" \
	-c "reset init" \
	-c "flash write_image erase %s" \
	-c "reset" \
	-c "shutdown"
""" % (env["OOCD_INTERFACE"], env["OOCD_TARGET"], str(elf[0])))

# And do something by default.
env.Alias("ubload", elf)
Default(elf, elfsize, rawbin)

