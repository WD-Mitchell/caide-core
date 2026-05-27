#include "../commands/history.h"
#include "../error.h"
#include "../references/naming.h"
#include "../internal.h"

extern "C" {

const char* caide_version(void) {
    return "1.0.0";
}

int caide_version_major(void) {
    return 1;
}

int caide_version_minor(void) {
    return 0;
}

int caide_version_patch(void) {
    return 0;
}

const char* caide_last_error_message(void) {
    return caide::last_error_message();
}

CaideError caide_last_error_code(void) {
    return caide::last_error_code();
}

CaideError caide_context_create(CaideContext* out_ctx) {
    if (out_ctx == nullptr) {
        return caide::set_error(CAIDE_ERR_INVALID_PARAM, "out_ctx cannot be null").code;
    }
    auto* context = new (std::nothrow) CaideContext_t();
    if (context == nullptr) {
        return caide::set_error(CAIDE_ERR_OUT_OF_MEMORY, "failed to allocate context handle").code;
    }
    context->impl = std::make_shared<caide::ContextData>();
    *out_ctx = context;
    caide::clear_error();
    return CAIDE_OK;
}

void caide_context_destroy(CaideContext ctx) {
    delete ctx;
    caide::clear_error();
}

CaideError caide_document_create(CaideContext ctx, CaideDocument* out_doc) {
    if (ctx == nullptr || ctx->impl == nullptr) {
        return caide::set_error(CAIDE_ERR_NULL_HANDLE, "context is null").code;
    }
    if (out_doc == nullptr) {
        return caide::set_error(CAIDE_ERR_INVALID_PARAM, "out_doc cannot be null").code;
    }
    auto* doc_handle = new (std::nothrow) CaideDocument_t();
    if (doc_handle == nullptr) {
        return caide::set_error(CAIDE_ERR_OUT_OF_MEMORY, "failed to allocate document handle").code;
    }
    doc_handle->impl = std::make_shared<caide::DocumentData>();
    {
        std::lock_guard<std::mutex> lock(ctx->impl->mutex);
        doc_handle->impl->document_id = "doc-" + std::to_string(ctx->impl->documents.size() + 1U);
        ctx->impl->documents.push_back(doc_handle->impl);
    }
    *out_doc = doc_handle;
    caide::clear_error();
    return CAIDE_OK;
}

void caide_document_destroy(CaideDocument doc) {
    delete doc;
    caide::clear_error();
}

CaideError caide_document_undo(CaideDocument doc) {
    if (doc == nullptr || doc->impl == nullptr) {
        return caide::set_error(CAIDE_ERR_NULL_HANDLE, "document is null").code;
    }
    return caide::commands::undo(*doc->impl).code;
}

CaideError caide_document_redo(CaideDocument doc) {
    if (doc == nullptr || doc->impl == nullptr) {
        return caide::set_error(CAIDE_ERR_NULL_HANDLE, "document is null").code;
    }
    return caide::commands::redo(*doc->impl).code;
}

int caide_document_history_count(CaideDocument doc) {
    if (doc == nullptr || doc->impl == nullptr) {
        caide::set_error(CAIDE_ERR_NULL_HANDLE, "document is null");
        return 0;
    }
    return caide::commands::history_count(*doc->impl);
}

CaideError caide_document_get_shape(CaideDocument doc, const char* ref_id, CaideShape* out_shape) {
    if (doc == nullptr || doc->impl == nullptr) {
        return caide::set_error(CAIDE_ERR_NULL_HANDLE, "document is null").code;
    }
    if (ref_id == nullptr || out_shape == nullptr) {
        return caide::set_error(CAIDE_ERR_INVALID_PARAM, "ref_id and out_shape are required").code;
    }
    auto shape = caide::commands::find_shape(*doc->impl, ref_id);
    if (!shape) {
        return shape.status.code;
    }
    *out_shape = caide::make_shape_handle(shape.value);
    caide::clear_error();
    return CAIDE_OK;
}

CaideError caide_reference_create(CaideShape shape, const char* label, CaideReference* out_ref) {
    if (shape == nullptr || shape->impl == nullptr) {
        return caide::set_error(CAIDE_ERR_NULL_HANDLE, "shape is null").code;
    }
    if (out_ref == nullptr) {
        return caide::set_error(CAIDE_ERR_INVALID_PARAM, "out_ref cannot be null").code;
    }
    auto reference = caide::references::create_reference(shape->impl, label != nullptr ? label : "");
    if (!reference) {
        return reference.status.code;
    }
    *out_ref = caide::make_reference_handle(reference.value);
    caide::clear_error();
    return CAIDE_OK;
}

void caide_reference_destroy(CaideReference ref) {
    delete ref;
    caide::clear_error();
}

CaideError caide_reference_resolve(CaideDocument doc, const char* ref_id, CaideShape* out_shape) {
    if (doc == nullptr || doc->impl == nullptr) {
        return caide::set_error(CAIDE_ERR_NULL_HANDLE, "document is null").code;
    }
    if (ref_id == nullptr || out_shape == nullptr) {
        return caide::set_error(CAIDE_ERR_INVALID_PARAM, "ref_id and out_shape are required").code;
    }
    auto result = caide::references::resolve(*doc->impl, ref_id);
    if (!result) {
        return result.status.code;
    }
    *out_shape = caide::make_shape_handle(result.value);
    caide::clear_error();
    return CAIDE_OK;
}

const char* caide_reference_get_id(CaideReference ref) {
    if (ref == nullptr || ref->impl == nullptr) {
        caide::set_error(CAIDE_ERR_NULL_HANDLE, "reference is null");
        return nullptr;
    }
    caide::clear_error();
    return ref->impl->id.c_str();
}

int caide_reference_is_valid(CaideReference ref) {
    if (ref == nullptr || ref->impl == nullptr) {
        caide::set_error(CAIDE_ERR_NULL_HANDLE, "reference is null");
        return 0;
    }
    return caide::references::is_reference_valid(ref->impl);
}

}  // extern "C"
