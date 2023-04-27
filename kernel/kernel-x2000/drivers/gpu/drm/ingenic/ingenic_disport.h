#ifndef __INGENIC_DISPORT_H__
#define __INGENIC_DISPORT_H__

extern struct drm_encoder *ingenic_disport_probe(struct device *dev);
extern int ingenic_disport_remove(struct drm_encoder *encoder);
extern int ingenic_disport_bind(struct drm_device *dev, struct drm_encoder *encoder);
#endif
