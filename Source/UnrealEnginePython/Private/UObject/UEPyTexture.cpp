#include "UEPyTexture.h"

#include "Runtime/Engine/Public/ImageUtils.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/Texture2D.h"
#include "Misc/App.h"

PyObject *py_ue_texture_update_resource(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);

	UTexture *texture = ue_py_check_type<UTexture>(self);
	if (!texture)
		return PyErr_Format(PyExc_Exception, "object is not a Texture");

	Py_BEGIN_ALLOW_THREADS;
	texture->UpdateResource();
	Py_END_ALLOW_THREADS;
	Py_RETURN_NONE;
}

PyObject *py_ue_texture_get_width(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);

	UTexture2D *texture = ue_py_check_type<UTexture2D>(self);
	if (!texture)
		return PyErr_Format(PyExc_Exception, "object is not a Texture");

	return PyLong_FromLong(texture->GetSizeX());
}

PyObject *py_ue_texture_get_height(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);

	UTexture2D *texture = ue_py_check_type<UTexture2D>(self);
	if (!texture)
		return PyErr_Format(PyExc_Exception, "object is not a Texture");

	return PyLong_FromLong(texture->GetSizeY());
}


PyObject* py_ue_texture_get_max_in_game_size(ue_PyUObject* self, PyObject* args)
{
	ue_py_check(self);

	UTexture* texture = ue_py_check_type<UTexture>(self);
	if (!texture)
		return PyErr_Format(PyExc_Exception, "object is not a Texture");
	
	if(!FApp::CanEverRender())
	{
		UE_LOG(LogPython, Warning, TEXT("py_ue_texture_get_max_in_game_size: Engine Cannot Ever Render, The Result is invalid."))
	}

	if(texture->GetResource() && texture->GetResource()->IsProxy())
	{
		Py_BEGIN_ALLOW_THREADS;
		texture->UpdateResource();
		Py_END_ALLOW_THREADS;
	}
	if(!texture->IsAsyncCacheComplete())
	{
		texture->FinishCachePlatformData();
	}
	
	const uint32 SurfaceWidth = (uint32)texture->GetSurfaceWidth();
	const uint32 SurfaceHeight = (uint32)texture->GetSurfaceHeight();
	const uint32 SurfaceDepth = (uint32)texture->GetSurfaceDepth();
	int32 MaxResMipBias = 0;
	UTexture2D* texture2d = Cast<UTexture2D>(texture);
	if (texture2d) 
	{
		MaxResMipBias = texture2d->GetNumMips() - texture2d->GetNumMipsAllowed(true);
	}
	else
	{
		MaxResMipBias = texture->GetCachedLODBias();
	}

	const uint32 MaxInGameWidth = FMath::Max<uint32>(SurfaceWidth >> MaxResMipBias, 1);
	const uint32 MaxInGameHeight = FMath::Max<uint32>(SurfaceHeight >> MaxResMipBias, 1);
	const uint32 MaxInGameDepth = FMath::Max<uint32>(SurfaceDepth >> MaxResMipBias, 1);
	
	PyObject* pyTuple = PyTuple_New(3);
	PyTuple_SetItem(pyTuple, 0, PyLong_FromUnsignedLong(MaxInGameWidth));
	PyTuple_SetItem(pyTuple, 1, PyLong_FromUnsignedLong(MaxInGameHeight));
	PyTuple_SetItem(pyTuple, 2, PyLong_FromUnsignedLong(MaxInGameDepth));

	return pyTuple;
}

PyObject *py_ue_texture_has_alpha_channel(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);

	UTexture2D *texture = ue_py_check_type<UTexture2D>(self);
	if (!texture)
		return PyErr_Format(PyExc_Exception, "object is not a Texture");

	if (texture->HasAlphaChannel())
		Py_RETURN_TRUE;
	Py_RETURN_FALSE;
}

PyObject *py_ue_texture_get_num_mips(ue_PyUObject *self, PyObject * args)
{
	ue_py_check(self);

	UTexture2D *texture = ue_py_check_type<UTexture2D>(self);
	if (!texture)
		return PyErr_Format(PyExc_Exception, "object is not a Texture");
	
	if(!FApp::CanEverRender())
	{
		UE_LOG(LogPython, Warning, TEXT("py_ue_texture_get_num_mips: Engine Cannot Ever Render, The Result is invalid."))
	}

	if(texture->GetResource()->IsProxy())
	{
		Py_BEGIN_ALLOW_THREADS;
		texture->UpdateResource();
		Py_END_ALLOW_THREADS;
	}
	if(!texture->IsAsyncCacheComplete())
	{
		texture->FinishCachePlatformData();
	}
	
	const int32 NumMips = texture->GetNumMips();
	return PyLong_FromLong(NumMips);
}

PyObject *py_ue_texture_get_platform_size(ue_PyUObject *self, PyObject * args)
{
	ue_py_check(self);

	PyObject* py_list = PyList_New(0);

	UTexture2D *texture = ue_py_check_type<UTexture2D>(self);
	if (!texture)
		return PyErr_Format(PyExc_Exception, "object is not a Texture");
	
	if(!FApp::CanEverRender())
	{
		UE_LOG(LogPython, Warning, TEXT("py_ue_texture_get_platform_size: Engine Cannot Ever Render, The Result is invalid."))
		PyObject* pyTuple = PyTuple_New(3);
		for (Py_ssize_t i = 0; i < 3; i++) {
			PyTuple_SetItem(pyTuple, i, PyLong_FromLong(-1));
		}
		return pyTuple;
	}
	if(texture->GetResource()->IsProxy())
	{
		Py_BEGIN_ALLOW_THREADS;
		texture->UpdateResource();
		Py_END_ALLOW_THREADS;
	}
	if(!texture->IsAsyncCacheComplete())
	{
		texture->FinishCachePlatformData();
	}
	
	int32 MaxResMipBias = texture->GetNumMips() - texture->GetNumMipsAllowed(true);
    uint32 nPlatformSizeX = FMath::Max<uint32>(texture->GetPlatformData()->SizeX >> MaxResMipBias, 1);
	uint32 nPlatformSizeY = FMath::Max<uint32>(texture->GetPlatformData()->SizeY >> MaxResMipBias, 1);
	int32 nSourceBytes = (int32)(CalcTextureSize(nPlatformSizeX, nPlatformSizeY, texture->GetPixelFormat(), texture->GetNumMips()) / 1024.0 + 0.5);

	PyList_Append(py_list, PyLong_FromUnsignedLong(nPlatformSizeX));
	PyList_Append(py_list, PyLong_FromUnsignedLong(nPlatformSizeY));
	PyList_Append(py_list, PyLong_FromLong(nSourceBytes));

	return py_list;
}

PyObject *py_ue_texture_get_data(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);

	int mipmap = 0;

	if (!PyArg_ParseTuple(args, "|i:texture_get_data", &mipmap))
	{
		return NULL;
	}

	UTexture2D *tex = ue_py_check_type<UTexture2D>(self);
	if (!tex)
		return PyErr_Format(PyExc_Exception, "object is not a Texture2D");
	
	if(!FApp::CanEverRender())
	{
		UE_LOG(LogPython, Warning, TEXT("py_ue_texture_get_data: Engine Cannot Ever Render, The Result is invalid."))
		PyObject* pyTuple = PyTuple_New(3);
		for (Py_ssize_t i = 0; i < 3; i++) {
			PyTuple_SetItem(pyTuple, i, PyLong_FromLong(-1));
		}
		return pyTuple;
	}

	if(tex->GetResource()->IsProxy())
	{
		Py_BEGIN_ALLOW_THREADS;
		tex->UpdateResource();
		Py_END_ALLOW_THREADS;
	}
	if(!tex->IsAsyncCacheComplete())
	{
		tex->FinishCachePlatformData();
	}

	if (mipmap >= tex->GetNumMips())
		return PyErr_Format(PyExc_Exception, "invalid mipmap id");

#if ENGINE_MAJOR_VERSION == 5
	const char *blob = (const char*)tex->GetPlatformData()->Mips[mipmap].BulkData.Lock(LOCK_READ_ONLY);
	PyObject *bytes = PyByteArray_FromStringAndSize(blob, (Py_ssize_t)tex->GetPlatformData()->Mips[mipmap].BulkData.GetBulkDataSize());
	tex->GetPlatformData()->Mips[mipmap].BulkData.Unlock();
#else
	const char *blob = (const char*)tex->PlatformData->Mips[mipmap].BulkData.Lock(LOCK_READ_ONLY);
	PyObject *bytes = PyByteArray_FromStringAndSize(blob, (Py_ssize_t)tex->PlatformData->Mips[mipmap].BulkData.GetBulkDataSize());
	tex->PlatformData->Mips[mipmap].BulkData.Unlock();
#endif
	return bytes;
}

#if WITH_EDITOR
PyObject *py_ue_texture_get_source_data(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);

	int mipmap = 0;

	if (!PyArg_ParseTuple(args, "|i:texture_get_data", &mipmap))
	{
		return nullptr;
	}

	UTexture2D *tex = ue_py_check_type<UTexture2D>(self);
	if (!tex)
		return PyErr_Format(PyExc_Exception, "object is not a Texture2D");

	if (mipmap >= tex->GetNumMips())
		return PyErr_Format(PyExc_Exception, "invalid mipmap id");

	const uint8 *blob = tex->Source.LockMip(mipmap);

	PyObject *bytes = PyByteArray_FromStringAndSize((const char *)blob, (Py_ssize_t)tex->Source.CalcMipSize(mipmap));

	tex->Source.UnlockMip(mipmap);
	return bytes;
}

PyObject *py_ue_texture_set_source_data(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);

	Py_buffer py_buf;
	int mipmap = 0;

	if (!PyArg_ParseTuple(args, "z*|i:texture_set_source_data", &py_buf, &mipmap))
	{
		return NULL;
	}

	UTexture2D *tex = ue_py_check_type<UTexture2D>(self);
	if (!tex)
	{
		PyBuffer_Release(&py_buf);
		return PyErr_Format(PyExc_Exception, "object is not a Texture2D");
	}


	if (!py_buf.buf)
	{
		PyBuffer_Release(&py_buf);
		return PyErr_Format(PyExc_Exception, "invalid data");
	}

	if (mipmap >= tex->GetNumMips())
	{
		PyBuffer_Release(&py_buf);
		return PyErr_Format(PyExc_Exception, "invalid mipmap id");
	}

	int32 wanted_len = py_buf.len;
	int32 len = tex->Source.GetSizeX() * tex->Source.GetSizeY() * 4;
	// avoid making mess
	if (wanted_len > len)
	{
		UE_LOG(LogPython, Warning, TEXT("truncating buffer to %d bytes"), len);
		wanted_len = len;
	}

	const uint8 *blob = tex->Source.LockMip(mipmap);

	FMemory::Memcpy((void *)blob, py_buf.buf, wanted_len);

	PyBuffer_Release(&py_buf);

	tex->Source.UnlockMip(mipmap);
	Py_BEGIN_ALLOW_THREADS;
	tex->MarkPackageDirty();
#if WITH_EDITOR
	tex->PostEditChange();
#endif

	tex->UpdateResource();
	Py_END_ALLOW_THREADS;
	Py_RETURN_NONE;
}
#endif

PyObject *py_ue_render_target_get_data(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);

	int mipmap = 0;

	if (!PyArg_ParseTuple(args, "|i:render_target_get_data", &mipmap))
	{
		return NULL;
	}

	UTextureRenderTarget2D *tex = ue_py_check_type<UTextureRenderTarget2D>(self);
	if (!tex)
		return PyErr_Format(PyExc_Exception, "object is not a TextureRenderTarget");


#if ENGINE_MAJOR_VERSION == 5
	FTextureRenderTarget2DResource *resource = (FTextureRenderTarget2DResource *)tex->GetResource();
#else
	FTextureRenderTarget2DResource *resource = (FTextureRenderTarget2DResource *)tex->Resource;
#endif
	if (!resource)
	{
		return PyErr_Format(PyExc_Exception, "cannot get render target resource");
	}

	TArray<FColor> pixels;

	if (!resource->IsSupportedFormat(tex->GetFormat()))
	{
		return PyErr_Format(PyExc_Exception, "unsupported format for render texture");
	}


	if (!resource->ReadPixels(pixels))
	{
		return PyErr_Format(PyExc_Exception, "unable to read pixels");
	}

	return PyByteArray_FromStringAndSize((const char *)pixels.GetData(), (Py_ssize_t)(pixels.GetTypeSize() * pixels.Num()));
}

PyObject *py_ue_render_target_get_data_to_buffer(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);
	Py_buffer py_buf;
	int mipmap = 0;

	if (!PyArg_ParseTuple(args, "z*|i:render_target_get_data_to_buffer", &py_buf, &mipmap))
	{
		return NULL;
	}

	UTextureRenderTarget2D *tex = ue_py_check_type<UTextureRenderTarget2D>(self);
	if (!tex)
	{
		PyBuffer_Release(&py_buf);
		return PyErr_Format(PyExc_Exception, "object is not a TextureRenderTarget");
	}

#if ENGINE_MAJOR_VERSION == 5
	FTextureRenderTarget2DResource *resource = (FTextureRenderTarget2DResource *)tex->GetResource();
#else
	FTextureRenderTarget2DResource *resource = (FTextureRenderTarget2DResource *)tex->Resource;
#endif
	if (!resource)
	{
		PyBuffer_Release(&py_buf);
		return PyErr_Format(PyExc_Exception, "cannot get render target resource");
	}

	Py_ssize_t data_len = (Py_ssize_t)(tex->GetSurfaceWidth() * 4 * tex->GetSurfaceHeight());
	if (py_buf.len < data_len)
	{
		PyBuffer_Release(&py_buf);
		return PyErr_Format(PyExc_Exception, "buffer is not big enough");
	}

	TArray<FColor> pixels;
	if (!resource->ReadPixels(pixels))
	{
		PyBuffer_Release(&py_buf);
		return PyErr_Format(PyExc_Exception, "unable to read pixels");
	}

	FMemory::Memcpy(py_buf.buf, pixels.GetData(), data_len);
	PyBuffer_Release(&py_buf);
	Py_RETURN_NONE;
}

PyObject *py_ue_texture_set_data(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);

	Py_buffer py_buf;
	int mipmap = 0;

	if (!PyArg_ParseTuple(args, "z*|i:texture_set_data", &py_buf, &mipmap))
	{
		return NULL;
	}

	UTexture2D *tex = ue_py_check_type<UTexture2D>(self);
	if (!tex)
	{
		PyBuffer_Release(&py_buf);
		return PyErr_Format(PyExc_Exception, "object is not a Texture2D");
	}
	
	if(!FApp::CanEverRender())
	{
		UE_LOG(LogPython, Warning, TEXT("py_ue_texture_set_data: Engine Cannot Ever Render, The Result is invalid."))
	}
	
	if(tex->GetResource()->IsProxy())
	{
		Py_BEGIN_ALLOW_THREADS;
		tex->UpdateResource();
		Py_END_ALLOW_THREADS;
	}
	if(!tex->IsAsyncCacheComplete())
	{
		tex->FinishCachePlatformData();
	}
	
	if (!py_buf.buf)
	{
		PyBuffer_Release(&py_buf);
		return PyErr_Format(PyExc_Exception, "invalid data");
	}

	if (mipmap >= tex->GetNumMips())
	{
		PyBuffer_Release(&py_buf);
		return PyErr_Format(PyExc_Exception, "invalid mipmap id");
	}

#if ENGINE_MAJOR_VERSION == 5
	char *blob = (char*)tex->GetPlatformData()->Mips[mipmap].BulkData.Lock(LOCK_READ_WRITE);
	int32 len = tex->GetPlatformData()->Mips[mipmap].BulkData.GetBulkDataSize();
#else
	char *blob = (char*)tex->PlatformData->Mips[mipmap].BulkData.Lock(LOCK_READ_WRITE);
	int32 len = tex->PlatformData->Mips[mipmap].BulkData.GetBulkDataSize();
#endif
	int32 wanted_len = py_buf.len;
	// avoid making mess
	if (wanted_len > len)
	{
		UE_LOG(LogPython, Warning, TEXT("truncating buffer to %d bytes"), len);
		wanted_len = len;
	}
	FMemory::Memcpy(blob, py_buf.buf, wanted_len);

	PyBuffer_Release(&py_buf);

#if ENGINE_MAJOR_VERSION == 5
	tex->GetPlatformData()->Mips[mipmap].BulkData.Unlock();
#else
	tex->PlatformData->Mips[mipmap].BulkData.Unlock();
#endif

	Py_BEGIN_ALLOW_THREADS;
	tex->MarkPackageDirty();
#if WITH_EDITOR
	tex->PostEditChange();
#endif

	tex->UpdateResource();
	Py_END_ALLOW_THREADS;

	Py_RETURN_NONE;
}

PyObject *py_unreal_engine_compress_image_array(PyObject * self, PyObject * args)
{
	int width;
	int height;
	Py_buffer py_buf;
	if (!PyArg_ParseTuple(args, "iiz*:compress_image_array", &width, &height, &py_buf))
	{
		return NULL;
	}

	if (py_buf.buf == nullptr || py_buf.len <= 0)
	{
		PyBuffer_Release(&py_buf);
		return PyErr_Format(PyExc_Exception, "invalid image data");
	}

	TArray<FColor> colors;
	uint8 *buf = (uint8 *)py_buf.buf;
	for (int32 i = 0; i < py_buf.len; i += 4)
	{
		colors.Add(FColor(buf[i], buf[1 + 1], buf[i + 2], buf[i + 3]));
	}

	PyBuffer_Release(&py_buf);

	TArray<uint8> output;

	Py_BEGIN_ALLOW_THREADS;
	FImageUtils::ThumbnailCompressImageArray(width, height, colors, output);
	Py_END_ALLOW_THREADS;

	return PyBytes_FromStringAndSize((char *)output.GetData(), output.Num());
}

PyObject *py_unreal_engine_create_checkerboard_texture(PyObject * self, PyObject * args)
{
	PyObject *py_color_one;
	PyObject *py_color_two;
	int checker_size;
	if (!PyArg_ParseTuple(args, "OOi:create_checkboard_texture", &py_color_one, &py_color_two, &checker_size))
	{
		return NULL;
	}

	ue_PyFColor *color_one = py_ue_is_fcolor(py_color_one);
	if (!color_one)
		return PyErr_Format(PyExc_Exception, "argument is not a FColor");

	ue_PyFColor *color_two = py_ue_is_fcolor(py_color_two);
	if (!color_two)
		return PyErr_Format(PyExc_Exception, "argument is not a FColor");

	UTexture2D *texture = nullptr;

	Py_BEGIN_ALLOW_THREADS;
	texture = FImageUtils::CreateCheckerboardTexture(color_one->color, color_two->color, checker_size);
	Py_END_ALLOW_THREADS;
	Py_RETURN_UOBJECT(texture);
}

PyObject *py_unreal_engine_create_transient_texture(PyObject * self, PyObject * args)
{
	int width;
	int height;
	int format = PF_B8G8R8A8;
	if (!PyArg_ParseTuple(args, "ii|i:create_transient_texture", &width, &height, &format))
	{
		return NULL;
	}


	UTexture2D *texture = UTexture2D::CreateTransient(width, height, (EPixelFormat)format);
	if (!texture)
		return PyErr_Format(PyExc_Exception, "unable to create texture");

	Py_BEGIN_ALLOW_THREADS;
	texture->UpdateResource();
	Py_END_ALLOW_THREADS;

	Py_RETURN_UOBJECT(texture);
}

PyObject *py_unreal_engine_create_transient_texture_render_target2d(PyObject * self, PyObject * args)
{
	int width;
	int height;
	int format = PF_B8G8R8A8;
	PyObject *py_linear = nullptr;
	if (!PyArg_ParseTuple(args, "ii|iO:create_transient_texture_render_target2d", &width, &height, &format, &py_linear))
	{
		return NULL;
	}

	UTextureRenderTarget2D *texture = NewObject<UTextureRenderTarget2D>(GetTransientPackage(), NAME_None, RF_Transient);
	if (!texture)
		return PyErr_Format(PyExc_Exception, "unable to create texture render target");

	Py_BEGIN_ALLOW_THREADS;
	texture->InitCustomFormat(width, height, (EPixelFormat)format, py_linear && PyObject_IsTrue(py_linear));
	Py_END_ALLOW_THREADS;
	Py_RETURN_UOBJECT(texture);
}

#if WITH_EDITOR
PyObject *py_unreal_engine_create_texture(PyObject * self, PyObject * args)
{
	PyObject *py_package;
	char *name;
	int width;
	int height;
	Py_buffer py_buf;

	if (!PyArg_ParseTuple(args, "Osiiz*:create_texture", &py_package, &name, &width, &height, &py_buf))
	{
		return nullptr;
	}

	UPackage *u_package = nullptr;
	if (py_package == Py_None)
	{
		u_package = GetTransientPackage();
	}
	else
	{
		u_package = ue_py_check_type<UPackage>(py_package);
		if (!u_package)
		{
			PyBuffer_Release(&py_buf);
			return PyErr_Format(PyExc_Exception, "argument is not a UPackage");
		}
	}

	SIZE_T wanted_len = width * height * 4;

	TArray<FColor> colors;
	colors.AddZeroed(wanted_len);
	FCreateTexture2DParameters params;

	if ((SIZE_T)py_buf.len < wanted_len)
		wanted_len = py_buf.len;

	FMemory::Memcpy(colors.GetData(), py_buf.buf, wanted_len);

	PyBuffer_Release(&py_buf);

	UTexture2D *texture = FImageUtils::CreateTexture2D(width, height, colors, u_package, UTF8_TO_TCHAR(name), RF_Public | RF_Standalone, params);
	if (!texture)
		return PyErr_Format(PyExc_Exception, "unable to create texture");

	Py_RETURN_UOBJECT(texture);
}
#endif

PyObject *py_ue_texture_get_pixel_format(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);
	
	UTexture2D *tex = ue_py_check_type<UTexture2D>(self);
	if (!tex)
		return PyErr_Format(PyExc_Exception, "object is not a Texture2D");

	if(!FApp::CanEverRender())
	{
		UE_LOG(LogPython, Warning, TEXT("py_ue_texture_get_pixel_format: Engine Cannot Ever Render, The Result is invalid."))
	}
	
	tex->FinishCachePlatformData();
	const EPixelFormat PF = tex->GetPlatformData()->PixelFormat;
	
	return PyLong_FromUnsignedLong(PF);
}