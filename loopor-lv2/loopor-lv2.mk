######################################
#
# loopor-lv2
#
######################################

# where to find the source code - locally in this case
LOOPOR_LV2_SITE_METHOD = local
LOOPOR_LV2_SITE = $($(PKG)_PKGDIR)/

# even though this is a local build, we still need a version number
# bump this number if you need to force a rebuild
LOOPOR_LV2_VERSION = 4

# dependencies (list of other buildroot packages, separated by space)
LOOPOR_LV2_DEPENDENCIES =

# LV2 bundles that this package generates (space separated list)
LOOPOR_LV2_BUNDLES = loopor.lv2

# call make with the current arguments and path. "$(@D)" is the build directory.
LOOPOR_LV2_TARGET_MAKE = $(TARGET_MAKE_ENV) $(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(@D)/source

# build command
define LOOPOR_LV2_BUILD_CMDS
	$(LOOPOR_LV2_TARGET_MAKE)
endef

# install command
define LOOPOR_LV2_INSTALL_TARGET_CMDS
	$(LOOPOR_LV2_TARGET_MAKE) install DESTDIR=$(TARGET_DIR)
endef


# import everything else from the buildroot generic package
$(eval $(generic-package))
