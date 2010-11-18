/* See LICENSE file for license and copyright information */

#include <stdlib.h>
#include <stdio.h>

#include "document.h"
#include "../utils.h"
#include "pdf/pdf.h"

#define LENGTH(x) (sizeof(x)/sizeof((x)[0]))

zathura_document_plugin_t zathura_document_plugins[] = {
  { "pdf", pdf_document_open },
};

zathura_document_t*
zathura_document_open(const char* path, const char* password)
{
  if(!path) {
    return NULL;
  }

  if(!file_exists(path)) {
    fprintf(stderr, "error: file does not exist\n");
    return NULL;
  }

  const char* file_extension = file_get_extension(path);
  if(!file_extension) {
    fprintf(stderr, "error: could not determine file type\n");
    return NULL;
  }

  zathura_document_t* document = malloc(sizeof(zathura_document_t));
  if(!document) {
    return NULL;
  }


  document->file_path           = path;
  document->password            = password;
  document->current_page_number = 0;
  document->number_of_pages     = 0;
  document->scale               = 100;
  document->rotate              = 0;

  document->functions.document_free            = NULL;
  document->functions.document_index_generate  = NULL;
  document->functions.document_save_as         = NULL;
  document->functions.document_attachments_get = NULL;
  document->functions.page_get                 = NULL;
  document->functions.page_free                = NULL;
  document->functions.page_search_text         = NULL;
  document->functions.page_links_get           = NULL;
  document->functions.page_form_fields_get     = NULL;
  document->functions.page_render              = NULL;

  /* init plugin with associated file type */
  for(unsigned int i = 0; i < LENGTH(zathura_document_plugins); i++)
  {

  }

  return NULL;
}

bool
zathura_document_free(zathura_document_t* document)
{
  if(!document) {
    return NULL;
  }

  if(!document->functions.document_free(document)) {
    fprintf(stderr, "error: %s not implemented\n", __FUNCTION__);
    return NULL;
  }

  bool r = document->functions.document_free(document);

  free(document);

  return r;
}

bool
zathura_document_save_as(zathura_document_t* document, const char* path)
{
  if(!document || !path) {
    return false;
  }

  if(!document->functions.document_save_as) {
    fprintf(stderr, "error: %s not implemented\n", __FUNCTION__);
    return false;
  }

  return document->functions.document_save_as(document, path);
}

zathura_list_t*
zathura_document_index_generate(zathura_document_t* document)
{
  if(!document) {
    return NULL;
  }

  if(!document->functions.document_index_generate) {
    fprintf(stderr, "error: %s not implemented\n", __FUNCTION__);
    return NULL;
  }

  return document->functions.document_index_generate(document);
}

bool
zathura_document_index_free(zathura_list_t* list)
{
  return false;
}

zathura_list_t*
zathura_document_attachments_get(zathura_document_t* document)
{
  if(!document) {
    return NULL;
  }

  if(!document->functions.document_attachments_get) {
    fprintf(stderr, "error: %s not implemented\n", __FUNCTION__);
    return NULL;
  }

  return document->functions.document_attachments_get(document);
}

bool
zathura_document_attachments_free(zathura_list_t* list)
{
  return false;
}

zathura_page_t*
zathura_page_get(zathura_document_t* document, unsigned int page)
{
  if(!document || !page) {
    return NULL;
  }

  if(!document->functions.page_get) {
    fprintf(stderr, "error: %s not implemented\n", __FUNCTION__);
    return NULL;
  }

  return document->functions.page_get(document, page);
}

bool
zathura_page_free(zathura_page_t* page)
{
  if(!page || !page->document) {
    return false;
  }

  if(!page->document->functions.page_free) {
    fprintf(stderr, "error: %s not implemented\n", __FUNCTION__);
    return false;
  }

  return page->document->functions.page_free(page);
}

zathura_list_t*
zathura_page_search_text(zathura_page_t* page, const char* text)
{
  if(!page || !page->document || !text) {
    return NULL;
  }

  if(!page->document->functions.page_search_text) {
    fprintf(stderr, "error: %s not implemented\n", __FUNCTION__);
    return NULL;
  }

  return page->document->functions.page_search_text(page, text);
}

zathura_list_t*
zathura_page_links_get(zathura_page_t* page)
{
  if(!page || !page->document) {
    return NULL;
  }

  if(!page->document->functions.page_links_get) {
    fprintf(stderr, "error: %s not implemented\n", __FUNCTION__);
    return NULL;
  }

  return page->document->functions.page_links_get(page);
}

bool
zathura_page_links_free(zathura_list_t* list)
{
  return false;
}

zathura_list_t*
zathura_page_form_fields_get(zathura_page_t* page)
{
  if(!page || !page->document) {
    return NULL;
  }

  if(!page->document->functions.page_form_fields_get) {
    fprintf(stderr, "error: %s not implemented\n", __FUNCTION__);
    return NULL;
  }

  return page->document->functions.page_form_fields_get(page);
}

bool
zathura_page_form_fields_free(zathura_list_t* list)
{
  return false;
}

cairo_surface_t*
zathura_page_render(zathura_page_t* page)
{
  if(!page || !page->document) {
    return NULL;
  }

  if(!page->document->functions.page_render) {
    fprintf(stderr, "error: %s not implemented\n", __FUNCTION__);
    return NULL;
  }

  return page->document->functions.page_render(page);
}
