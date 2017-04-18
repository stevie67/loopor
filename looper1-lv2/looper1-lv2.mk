######################################
#
# looper1-lv2
#
######################################

# where to find the source code - locally in this case
LOOPER1_LV2_SITE_METHOD = local
LOOPER1_LV2_SITE = $($(PKG)_PKGDIR)/

# even though this is a local build, we still need a version number
# bump this number if you need to force a rebuild
LOOPER1_LV2_VERSION = 67

# dependencies (list of other buildroot packages, separated by space)
LOOPER1_LV2_DEPENDENCIES =

# LV2 bundles that this package generates (space separated list)
LOOPER1_LV2_BUNDLES = looper1.lv2

# call make with the current arguments and path. "$(@D)" is the build directory.
LOOPER1_LV2_TARGET_MAKE = $(TARGET_MAKE_ENV) $(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(@D)/source


# build command
define LOOPER1_LV2_BUILD_CMDS
	$(LOOPER1_LV2_TARGET_MAKE)
endef

# install command
define LOOPER1_LV2_INSTALL_TARGET_CMDS
	$(LOOPER1_LV2_TARGET_MAKE) install DESTDIR=$(TARGET_DIR)
endef


# import everything else from the buildroot generic package
$(eval $(generic-package))
