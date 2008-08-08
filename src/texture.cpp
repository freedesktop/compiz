/*
 * Copyright © 2005 Novell, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Novell, Inc. not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.
 * Novell, Inc. makes no representations about the suitability of this
 * software for any purpose. It is provided "as is" without express or
 * implied warranty.
 *
 * NOVELL, INC. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL NOVELL, INC. BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: David Reveman <davidr@novell.com>
 */

#ifdef HAVE_CONFIG_H
#  include "../config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <compiz-core.h>
#include "privatescreen.h"

static CompMatrix _identity_matrix = {
    1.0f, 0.0f,
    0.0f, 1.0f,
    0.0f, 0.0f
};

void
initTexture (CompScreen  *screen,
	     CompTexture *texture)
{
    texture->refCount	= 1;
    texture->name	= 0;
    texture->target	= GL_TEXTURE_2D;
    texture->pixmap	= None;
    texture->filter	= GL_NEAREST;
    texture->wrap	= GL_CLAMP_TO_EDGE;
    texture->matrix     = _identity_matrix;
    texture->oldMipmaps = TRUE;
    texture->mipmap	= FALSE;
}

void
finiTexture (CompScreen  *screen,
	     CompTexture *texture)
{
    if (texture->name)
    {
	screen->makeCurrent ();
	screen->releasePixmapFromTexture (texture);
	glDeleteTextures (1, &texture->name);
    }
}

CompTexture *
createTexture (CompScreen *screen)
{
    CompTexture *texture;

    texture = (CompTexture *) malloc (sizeof (CompTexture));
    if (!texture)
	return NULL;

    initTexture (screen, texture);

    return texture;
}

void
destroyTexture (CompScreen  *screen,
		CompTexture *texture)
{
    texture->refCount--;
    if (texture->refCount)
	return;

    finiTexture (screen, texture);

    free (texture);
}

static Bool
imageToTexture (CompScreen   *screen,
		CompTexture  *texture,
		const char   *image,
		unsigned int width,
		unsigned int height,
		GLenum       format,
		GLenum       type)
{
    char     *data;
    unsigned int i;
    GLint    internalFormat;

    data = (char *) malloc (4 * width * height);
    if (!data)
	return FALSE;

    for (i = 0; i < height; i++)
	memcpy (&data[i * width * 4],
		&image[(height - i - 1) * width * 4],
		width * 4);

    screen->makeCurrent ();
    screen->releasePixmapFromTexture (texture);

    if (screen->textureNonPowerOfTwo () ||
	(POWER_OF_TWO (width) && POWER_OF_TWO (height)))
    {
	texture->target = GL_TEXTURE_2D;
	texture->matrix.xx = 1.0f / width;
	texture->matrix.yy = -1.0f / height;
	texture->matrix.y0 = 1.0f;
	texture->mipmap = TRUE;
    }
    else
    {
	texture->target = GL_TEXTURE_RECTANGLE_NV;
	texture->matrix.xx = 1.0f;
	texture->matrix.yy = -1.0f;
	texture->matrix.y0 = height;
	texture->mipmap = FALSE;
    }

    if (!texture->name)
	glGenTextures (1, &texture->name);

    glBindTexture (texture->target, texture->name);

    internalFormat =
	(screen->getOption ("texture_compression")->value.b &&
	 screen->textureCompression () ?
	 GL_COMPRESSED_RGBA_ARB : GL_RGBA);

    glTexImage2D (texture->target, 0, internalFormat, width, height, 0,
		  format, type, data);

    texture->filter = GL_NEAREST;

    glTexParameteri (texture->target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri (texture->target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexParameteri (texture->target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri (texture->target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    texture->wrap = GL_CLAMP_TO_EDGE;

    glBindTexture (texture->target, 0);

    free (data);

    return TRUE;
}

Bool
imageBufferToTexture (CompScreen   *screen,
		      CompTexture  *texture,
		      const char   *image,
		      unsigned int width,
		      unsigned int height)
{
#if IMAGE_BYTE_ORDER == MSBFirst
    return imageToTexture (screen, texture, image, width, height,
			   GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV);
#else
    return imageToTexture (screen, texture, image, width, height,
			   GL_BGRA, GL_UNSIGNED_BYTE);
#endif
}

Bool
imageDataToTexture (CompScreen   *screen,
		    CompTexture  *texture,
		    const char	   *image,
		    unsigned int width,
		    unsigned int height,
		    GLenum       format,
		    GLenum       type)
{
    return imageToTexture (screen, texture, image, width, height, format, type);
}


Bool
readImageToTexture (CompScreen   *screen,
		    CompTexture  *texture,
		    const char	 *imageFileName,
		    unsigned int *returnWidth,
		    unsigned int *returnHeight)
{
    void *image;
    int  width, height;
    Bool status;

    if (screen->display ()->readImageFromFile (imageFileName, &width, &height,
					    &image))
	return false;

    status = imageBufferToTexture (screen, texture, (char *)image, width, height);

    free (image);

    if (returnWidth)
	*returnWidth = width;
    if (returnHeight)
	*returnHeight = height;

    return status;
}

Bool
iconToTexture (CompScreen *screen,
	       CompIcon   *icon)
{
    return imageBufferToTexture (screen, &icon->texture,
				 (char *) (icon + 1),
				 icon->width,
				 icon->height);
}

bool
CompScreen::bindPixmapToTexture (CompTexture *texture,
				 Pixmap      pixmap,
				 int         width,
				 int         height,
				 int         depth)
{
    unsigned int target = 0;
    CompFBConfig *config = &priv->glxPixmapFBConfigs[depth];
    int          attribs[7], i = 0;

    if (!config->fbConfig)
    {
	compLogMessage (NULL, "core", CompLogLevelWarn,
			"No GLXFBConfig for depth %d",
			depth);

	return false;
    }

    attribs[i++] = GLX_TEXTURE_FORMAT_EXT;
    attribs[i++] = config->textureFormat;
    attribs[i++] = GLX_MIPMAP_TEXTURE_EXT;
    attribs[i++] = config->mipmap;

    /* If no texture target is specified in the fbconfig, or only the
       TEXTURE_2D target is specified and GL_texture_non_power_of_two
       is not supported, then allow the server to choose the texture target. */
    if (config->textureTargets & GLX_TEXTURE_2D_BIT_EXT &&
       (priv->textureNonPowerOfTwo ||
       (POWER_OF_TWO (width) && POWER_OF_TWO (height))))
	target = GLX_TEXTURE_2D_EXT;
    else if (config->textureTargets & GLX_TEXTURE_RECTANGLE_BIT_EXT)
	target = GLX_TEXTURE_RECTANGLE_EXT;

    /* Workaround for broken texture from pixmap implementations, 
       that don't advertise any texture target in the fbconfig. */
    if (!target)
    {
	if (!(config->textureTargets & GLX_TEXTURE_2D_BIT_EXT))
	    target = GLX_TEXTURE_RECTANGLE_EXT;
	else if (!(config->textureTargets & GLX_TEXTURE_RECTANGLE_BIT_EXT))
	    target = GLX_TEXTURE_2D_EXT;
    }

    if (target)
    {
	attribs[i++] = GLX_TEXTURE_TARGET_EXT;
	attribs[i++] = target;
    }

    attribs[i++] = None;

    makeCurrent ();
    texture->pixmap = (*createPixmap) (priv->display->dpy (),
				       config->fbConfig, pixmap,
				       attribs);
    if (!texture->pixmap)
    {
	compLogMessage (NULL, "core", CompLogLevelWarn,
			"glXCreatePixmap failed");

	return false;
    }

    if (!target)
	(*queryDrawable) (priv->display->dpy (),
			  texture->pixmap,
			  GLX_TEXTURE_TARGET_EXT,
			  &target);

    switch (target) {
    case GLX_TEXTURE_2D_EXT:
	texture->target = GL_TEXTURE_2D;

	texture->matrix.xx = 1.0f / width;
	if (config->yInverted)
	{
	    texture->matrix.yy = 1.0f / height;
	    texture->matrix.y0 = 0.0f;
	}
	else
	{
	    texture->matrix.yy = -1.0f / height;
	    texture->matrix.y0 = 1.0f;
	}
	texture->mipmap = config->mipmap;
	break;
    case GLX_TEXTURE_RECTANGLE_EXT:
	texture->target = GL_TEXTURE_RECTANGLE_ARB;

	texture->matrix.xx = 1.0f;
	if (config->yInverted)
	{
	    texture->matrix.yy = 1.0f;
	    texture->matrix.y0 = 0;
	}
	else
	{
	    texture->matrix.yy = -1.0f;
	    texture->matrix.y0 = height;
	}
	texture->mipmap = false;
	break;
    default:
	compLogMessage (NULL, "core", CompLogLevelWarn,
			"pixmap 0x%x can't be bound to texture",
			(int) pixmap);

	glXDestroyGLXPixmap (priv->display->dpy (), texture->pixmap);
	texture->pixmap = None;

	return false;
    }

    if (!texture->name)
	glGenTextures (1, &texture->name);

    glBindTexture (texture->target, texture->name);

    if (!strictBinding)
    {
	(*bindTexImage) (priv->display->dpy (),
			 texture->pixmap,
			 GLX_FRONT_LEFT_EXT,
			 NULL);
    }

    texture->filter = GL_NEAREST;

    glTexParameteri (texture->target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri (texture->target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexParameteri (texture->target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri (texture->target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    texture->wrap = GL_CLAMP_TO_EDGE;

    glBindTexture (texture->target, 0);

    return true;
}

void
CompScreen::releasePixmapFromTexture (CompTexture *texture)
{
    if (texture->pixmap)
    {
	makeCurrent ();
	glEnable (texture->target);
	if (!strictBinding)
	{
	    glBindTexture (texture->target, texture->name);

	    (*releaseTexImage) (priv->display->dpy (),
				texture->pixmap,
				GLX_FRONT_LEFT_EXT);
	}

	glBindTexture (texture->target, 0);
	glDisable (texture->target);

	glXDestroyGLXPixmap (priv->display->dpy (), texture->pixmap);

	texture->pixmap = None;
    }
}

void
CompScreen::enableTexture (CompTexture	 *texture,
			   CompTextureFilter filter)
{
    makeCurrent ();
    glEnable (texture->target);
    glBindTexture (texture->target, texture->name);

    if (strictBinding && texture->pixmap)
    {
	(*bindTexImage) (priv->display->dpy (),
			 texture->pixmap,
			 GLX_FRONT_LEFT_EXT,
			 NULL);
    }

    if (filter == COMP_TEXTURE_FILTER_FAST)
    {
	if (texture->filter != GL_NEAREST)
	{
	    glTexParameteri (texture->target,
			     GL_TEXTURE_MIN_FILTER,
			     GL_NEAREST);
	    glTexParameteri (texture->target,
			     GL_TEXTURE_MAG_FILTER,
			     GL_NEAREST);

	    texture->filter = GL_NEAREST;
	}
    }
    else if (texture->filter != priv->display->textureFilter ())
    {
	if (priv->display->textureFilter () == GL_LINEAR_MIPMAP_LINEAR)
	{
	    if (priv->textureNonPowerOfTwo && priv->fbo && texture->mipmap)
	    {
		glTexParameteri (texture->target,
				 GL_TEXTURE_MIN_FILTER,
				 GL_LINEAR_MIPMAP_LINEAR);

		if (texture->filter != GL_LINEAR)
		    glTexParameteri (texture->target,
				     GL_TEXTURE_MAG_FILTER,
				     GL_LINEAR);

		texture->filter = GL_LINEAR_MIPMAP_LINEAR;
	    }
	    else if (texture->filter != GL_LINEAR)
	    {
		glTexParameteri (texture->target,
				 GL_TEXTURE_MIN_FILTER,
				 GL_LINEAR);
		glTexParameteri (texture->target,
				 GL_TEXTURE_MAG_FILTER,
				 GL_LINEAR);

		texture->filter = GL_LINEAR;
	    }
	}
	else
	{
	    glTexParameteri (texture->target,
			     GL_TEXTURE_MIN_FILTER,
			     priv->display->textureFilter ());
	    glTexParameteri (texture->target,
			     GL_TEXTURE_MAG_FILTER,
			     priv->display->textureFilter ());

	    texture->filter = priv->display->textureFilter ();
	}
    }

    if (texture->filter == GL_LINEAR_MIPMAP_LINEAR)
    {
	if (texture->oldMipmaps)
	{
	    (*generateMipmap) (texture->target);
	    texture->oldMipmaps = FALSE;
	}
    }
}

void
CompScreen::disableTexture (CompTexture *texture)
{
    makeCurrent ();
    if (strictBinding && texture->pixmap)
    {
	glBindTexture (texture->target, texture->name);

	(*releaseTexImage) (priv->display->dpy (),
			    texture->pixmap,
			    GLX_FRONT_LEFT_EXT);
    }

    glBindTexture (texture->target, 0);
    glDisable (texture->target);
}