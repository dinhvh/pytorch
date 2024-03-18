#include <torch/csrc/dynamo/init.h>

#include <torch/csrc/Exceptions.h>
#include <torch/csrc/dynamo/cache_entry.h>
#include <torch/csrc/dynamo/eval_frame.h>
#include <torch/csrc/dynamo/extra_state.h>
#include <torch/csrc/dynamo/guards.h>
#include <torch/csrc/dynamo/python_compiled_autograd.h>
#include <torch/csrc/utils/python_raii.h>

static struct PyModuleDef _module =
    {PyModuleDef_HEAD_INIT, "torch._C._dynamo", "", -1, nullptr};

namespace torch {
namespace dynamo {
using torch::dynamo::autograd::torch_c_dynamo_compiled_autograd_init;

namespace {

// An RAII to save and restore the current local dispatch keyset. Shouldn't
// normally be needed - mostly for emergency use.
struct PreserveDispatch {
  c10::impl::LocalDispatchKeySet m_old;

  ~PreserveDispatch() {
    c10::impl::_force_tls_local_dispatch_key_set(m_old);
  }

  PreserveDispatch() : m_old(c10::impl::tls_local_dispatch_key_set()) {}
};

} // anonymous namespace

void initDynamoBindings(PyObject* torch) {
  PyObject* dynamo = PyModule_Create(&_module);
  if (dynamo == nullptr || PyModule_AddObject(torch, "_dynamo", dynamo) != 0) {
    throw python_error();
  }

  PyObject* eval_frame = torch_c_dynamo_eval_frame_init();
  if (eval_frame == nullptr ||
      PyModule_AddObject(dynamo, "eval_frame", eval_frame) != 0) {
    throw python_error();
  }

  PyObject* guards = torch_c_dynamo_guards_init();
  if (guards == nullptr || PyModule_AddObject(dynamo, "guards", guards) != 0) {
    throw python_error();
  }

  PyObject* compiled_autograd = torch_c_dynamo_compiled_autograd_init();
  if (compiled_autograd == nullptr ||
      PyModule_AddObject(dynamo, "compiled_autograd", compiled_autograd) != 0) {
    throw python_error();
  }

  torch::impl::py_context_manager<PreserveDispatch>(
      py::handle(dynamo).cast<py::module>(), "_PreserveDispatch");

  auto m = py::handle(eval_frame).cast<py::module>();

  py::class_<CacheEntry>(m, "_CacheEntry")
      .def_readonly("check_fn", &CacheEntry::check_fn)
      .def_readonly("code", &CacheEntry::code)
      .def_property_readonly("next", &CacheEntry::next);

  py::class_<ExtraState>(m, "_ExtraState")
      .def("invalidate", &ExtraState::invalidate);

  m.def("_debug_get_cache_entry_list", &_debug_get_cache_entry_list);
}

} // namespace dynamo
} // namespace torch
