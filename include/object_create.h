/// \file
/// Declares creation of individual runtime objects from borrowed templates.
#ifndef OBJECT_CREATE_H
#define OBJECT_CREATE_H
#include "object_template.h"

/// Copy a template and initialize the runtime kind's resources.
/// \param definition Readable complete template with a valid runtime kind and its required resources.
/// \returns Newly owned object; remove active links before releasing it with objectDestroy.
/// Template storage is borrowed for this call. Referenced model resources must remain valid while used.
/// This creates one object; hierarchy links require explicit setup or hierarchy cloning.
u8 *objectCreateFromTemplate(ObjectTemplate *definition);
#endif
