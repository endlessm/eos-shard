/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */
/*
 * eos-shard-file.c
 *
 * Copyright (C) 2016 Endless Mobile, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Author: Juan Pablo Ugarte <ugarte@endlessm.com>
 *
 */

#include "eos-shard-file.h"
#include "eos-shard-file-input-stream-wrapper.h"

#define EOS_SHARD_SCHEME_LEN 6

struct _EosShardFile
{
  GObject parent;
};

typedef struct
{
  gchar *uri;
  gchar *uri_scheme;
  EosShardBlob *blob;
} EosShardFilePrivate;

enum
{
  PROP_0,

  PROP_URI,
  PROP_URI_SCHEME,
  PROP_BLOB,
  N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES];

static void eos_shard_file_iface_init (gpointer g_iface, gpointer iface_data);

G_DEFINE_TYPE_EXTENDED (EosShardFile,
			                  eos_shard_file,
			                  G_TYPE_OBJECT, 0,
			                  G_ADD_PRIVATE (EosShardFile)
			                  G_IMPLEMENT_INTERFACE (G_TYPE_FILE,
                                               eos_shard_file_iface_init))

#define EOS_SHARD_FILE_PRIVATE(d) ((EosShardFilePrivate *) eos_shard_file_get_instance_private((EosShardFile*)d))

static void
eos_shard_file_init (EosShardFile *self)
{

}

static void
eos_shard_file_finalize (GObject *self)
{
  EosShardFilePrivate *priv = EOS_SHARD_FILE_PRIVATE (self);

  g_clear_pointer (&priv->uri, g_free);
  g_clear_pointer (&priv->blob, eos_shard_blob_unref);
  
  G_OBJECT_CLASS (eos_shard_file_parent_class)->finalize (self);
}

static inline void
eos_shard_file_set_uri (EosShardFile *self, const gchar *uri)
{
  EosShardFilePrivate *priv = EOS_SHARD_FILE_PRIVATE (self);

  if (priv->uri_scheme)
    g_return_if_fail (g_str_has_prefix (uri, priv->uri_scheme));

  if (g_strcmp0 (priv->uri, uri) != 0)
    {
      g_free (priv->uri);
      priv->uri = g_strdup (uri);
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_URI]);
    }
}

static inline void
eos_shard_file_set_uri_scheme (EosShardFile *self, const gchar *uri_scheme)
{
  EosShardFilePrivate *priv = EOS_SHARD_FILE_PRIVATE (self);

  if (priv->uri)
    g_return_if_fail (g_str_has_prefix (priv->uri, uri_scheme));

  if (g_strcmp0 (priv->uri_scheme, uri_scheme) != 0)
    {
      g_free (priv->uri_scheme);
      priv->uri_scheme = g_strdup (uri_scheme);
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_URI_SCHEME]);
    }
}

static inline void
eos_shard_file_set_blob (EosShardFile *self, EosShardBlob *blob)
{
  EosShardFilePrivate *priv = EOS_SHARD_FILE_PRIVATE (self);

  g_clear_pointer (&priv->blob, eos_shard_blob_unref);
  
  if (blob)
    priv->blob = eos_shard_blob_ref (blob);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_BLOB]);
}

static void
eos_shard_file_set_property (GObject      *self,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  g_return_if_fail (EOS_SHARD_IS_FILE (self));

  switch (prop_id)
    {
    case PROP_URI:
      eos_shard_file_set_uri (EOS_SHARD_FILE (self), g_value_get_string (value));
      break;
    case PROP_URI_SCHEME:
      eos_shard_file_set_uri_scheme (EOS_SHARD_FILE (self), g_value_get_string (value));
      break;
    case PROP_BLOB:
      eos_shard_file_set_blob (EOS_SHARD_FILE (self), g_value_get_pointer (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, prop_id, pspec);
      break;
    }
}

static void
eos_shard_file_get_property (GObject    *self,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  EosShardFilePrivate *priv;

  g_return_if_fail (EOS_SHARD_IS_FILE (self));
  priv = EOS_SHARD_FILE_PRIVATE (self);

  switch (prop_id)
    {
    case PROP_URI:
      g_value_set_string (value, priv->uri);
      break;
    case PROP_URI_SCHEME:
      g_value_set_string (value, priv->uri_scheme);
      break;
    case PROP_BLOB:
      g_value_set_pointer (value, priv->blob);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, prop_id, pspec);
      break;
    }
}

/* GFile iface implementation */

static GFile *
eos_shard_file_dup (GFile *self)
{
  EosShardFilePrivate *priv = EOS_SHARD_FILE_PRIVATE (self);
  return eos_shard_file_new (priv->uri, priv->uri_scheme, priv->blob);
}

static guint
eos_shard_file_hash (GFile *self)
{
  return g_str_hash (EOS_SHARD_FILE_PRIVATE (self)->uri);
}

static gboolean
eos_shard_file_equal (GFile *file1, GFile *file2)
{
  return g_str_equal (EOS_SHARD_FILE_PRIVATE (file1)->uri, EOS_SHARD_FILE_PRIVATE (file2)->uri);
}

static gboolean
eos_shard_file_is_native (GFile *self)
{
  return TRUE;
}

static gboolean 
eos_shard_file_has_uri_scheme (GFile *self, const char *uri_scheme)
{
  EosShardFilePrivate *priv = EOS_SHARD_FILE_PRIVATE (self);
  return g_str_equal (priv->uri_scheme, uri_scheme);
}

static char *
eos_shard_file_get_uri_scheme (GFile *self)
{
  EosShardFilePrivate *priv = EOS_SHARD_FILE_PRIVATE (self);
  return g_strdup (priv->uri_scheme);
}

static char *
eos_shard_file_get_basename (GFile *self)
{
  EosShardFilePrivate *priv = EOS_SHARD_FILE_PRIVATE (self);
  return priv->uri ? g_path_get_basename (priv->uri + EOS_SHARD_SCHEME_LEN) : NULL;
}

static char *
eos_shard_file_get_path (GFile *self)
{
  EosShardFilePrivate *priv = EOS_SHARD_FILE_PRIVATE (self);
  return priv->uri ? g_strdup (priv->uri + EOS_SHARD_SCHEME_LEN) : NULL;
}

static char *
eos_shard_file_get_uri (GFile *self)
{
  return g_strdup (EOS_SHARD_FILE_PRIVATE (self)->uri);
}

static char *
eos_shard_file_get_parse_name (GFile *self)
{
  return g_strdup (EOS_SHARD_FILE_PRIVATE (self)->uri);
}

static GFile *
eos_shard_file_get_parent (GFile *self)
{
  return NULL;
}

static GFileInfo *
eos_shard_file_query_info (GFile                *self,
                           const char           *attributes,
                           GFileQueryInfoFlags   flags,
                           GCancellable         *cancellable,
                           GError              **error)
{
  EosShardFilePrivate *priv = EOS_SHARD_FILE_PRIVATE (self);
  GFileAttributeMatcher *matcher;
  GFileInfo *info;

  info    = g_file_info_new ();
  matcher = g_file_attribute_matcher_new (attributes);

  if (g_file_attribute_matcher_matches (matcher, G_FILE_ATTRIBUTE_STANDARD_SIZE))
    g_file_info_set_size (info, eos_shard_blob_get_content_size (priv->blob));

  if (g_file_attribute_matcher_matches (matcher, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE))
    g_file_info_set_content_type (info, eos_shard_blob_get_content_type (priv->blob));

  g_file_attribute_matcher_unref (matcher);

  return info;
}

static GFileInputStream *
eos_shard_file_read_fn (GFile *self, GCancellable *cancellable, GError **error)
{
  GInputStream *stream = eos_shard_blob_get_stream (EOS_SHARD_FILE_PRIVATE (self)->blob);
  return _eos_shard_file_input_stream_wrapper_new (self, stream);
}

static void
eos_shard_file_iface_init (gpointer g_iface, gpointer iface_data)
{
  GFileIface *iface = g_iface;

  iface->dup            = eos_shard_file_dup;
  iface->hash           = eos_shard_file_hash;
  iface->equal          = eos_shard_file_equal;
  iface->is_native      = eos_shard_file_is_native;
  iface->has_uri_scheme = eos_shard_file_has_uri_scheme;
  iface->get_uri_scheme = eos_shard_file_get_uri_scheme;
  iface->get_basename   = eos_shard_file_get_basename;
  iface->get_path       = eos_shard_file_get_path;
  iface->get_uri        = eos_shard_file_get_uri;
  iface->get_parse_name = eos_shard_file_get_parse_name;
  iface->get_parent     = eos_shard_file_get_parent;
  iface->query_info     = eos_shard_file_query_info;
  iface->read_fn        = eos_shard_file_read_fn;
}

static void
eos_shard_file_class_init (EosShardFileClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize     = eos_shard_file_finalize;
  object_class->get_property = eos_shard_file_get_property;
  object_class->set_property = eos_shard_file_set_property;

  /* Properties */
  properties[PROP_URI] =
    g_param_spec_string ("uri",
                         "URI",
                         "URI to associate this file with",
                         NULL,
                         G_PARAM_READWRITE |
                         G_PARAM_CONSTRUCT_ONLY |
                         G_PARAM_STATIC_STRINGS);

  properties[PROP_URI_SCHEME] =
    g_param_spec_string ("uri-scheme",
                         "URI Scheme",
                         "URI Scheme to use",
                         NULL,
                         G_PARAM_READWRITE |
                         G_PARAM_CONSTRUCT_ONLY |
                         G_PARAM_STATIC_STRINGS);

  properties[PROP_BLOB] =
    g_param_spec_pointer ("blob",
                          "Blob",
                          "EosShardBlob for uri",
                          G_PARAM_READWRITE |
                          G_PARAM_CONSTRUCT_ONLY |
                          G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPERTIES, properties);
}

GFile *
eos_shard_file_new (const gchar *uri,
                    const gchar *uri_scheme,
                    EosShardBlob *blob)
{
  return g_object_new (EOS_SHARD_TYPE_FILE,
                       "uri", uri,
                       "uri-scheme", uri_scheme,
                       "blob", blob,
                       NULL);
}
