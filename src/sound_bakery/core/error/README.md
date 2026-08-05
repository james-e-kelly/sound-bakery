# Error handling

Sound Bakery has two error "currencies":

| Type | What it is | Where it lives |
| --- | --- | --- |
| `sbk_result` | A plain C status **code** (an `enum`). | The C ABI: every `sc_*` and public `sbk_*` function. |
| `sbk::result<T>` | A rich result: a value **or** an `sbk::error` (code + message + source location). Alias for `tl::expected<T, sbk::error>`. | Internal C++ code. |

**Rule of thumb:** prefer `sbk::result<T>` for new C++ code. Only return the raw
`sbk_result` code when you must — i.e. at the C ABI boundary, or in older code not
yet migrated. Convert between them at the boundary with `sbk::to_status(...)`.

Failures are **logged once, at the origin** (the spot they are first produced). Forwarding
an error up the stack does not log again, so one failure produces exactly one log line.
You never write `if (failed) log(...)` yourself — the macros below do it.

## Picking a macro

Two questions decide it:

1. **What does the function I'm writing return?** → picks the *prefix*.
2. **Am I guarding a condition, or calling something fallible?** → picks the *verb*.

| Your function returns... | Prefix |
| --- | --- |
| `sbk::result<T>` (preferred) | *(none)* — `SBK_TRY`, `SBK_CHECK`, `SBK_FAIL` |
| `sbk_result` (C code) | `SBK_STATUS_` — `SBK_STATUS_TRY_C`, `SBK_STATUS_CHECK`, `SBK_STATUS_FAIL` |
| `sbk_id` (legacy lookups) | `SBK_ID_` — `SBK_ID_CHECK` |

| Verb | Meaning |
| --- | --- |
| `CHECK(cond, code)` | Guard a bool. If false: log + return `code` as a failure. |
| `TRY(var, expr)` / `TRY_C(expr)` | Run a fallible call. On failure: log at origin + forward. `TRY` unwraps a `sbk::result`'s value into `var`; `TRY_C` wraps a C `sbk_result` call. |
| `FAIL(code, ...)` | Unconditionally log + return a failure (e.g. an unreachable branch). |

| Suffix | Meaning |
| --- | --- |
| `_MSG` | Adds a **fmt-style** message + args for context the code alone can't convey. |
| `_C` | The thing being `TRY`'d is a C function returning `sbk_result`. |

Base (non-`_MSG`) macros auto-fill the message with the stringized condition/expression,
so only reach for `_MSG` when a human sentence adds information.

## Full list

**Family 1 — your function returns `sbk::result<T>`** (preferred):

```cpp
SBK_TRY(auto value, some_result_returning_fn());  // unwrap value, or forward the error
SBK_TRYV(some_result_void_fn());                  // run it, or forward the error (no value)
SBK_TRY_C(sc_system_init(sys, &cfg));             // wrap a C sbk_result call, forward as error
SBK_TRY_C_MSG(sc_..(..), "while starting '{}'", name);
SBK_CHECK(ptr != nullptr, SBK_ERR_NULL);          // guard
SBK_CHECK_MSG(ok, SBK_ERR_BAKERY, "bad state for '{}'", name);
SBK_FAIL(SBK_ERR_BAKERY, "unreachable: type {}", n);
return sbk::ok();                                 // success for result<void>
```

**Family 2 — your function returns `sbk_result`** (C ABI / legacy):

```cpp
SBK_STATUS_CHECK(ptr != nullptr, SBK_ERR_NULL);
SBK_STATUS_CHECK_MSG(ok, SBK_ERR_BAKERY, "bad state for '{}'", name);
SBK_STATUS_TRY_C(sc_system_init(sys, &cfg));        // wrap a C call, return its code
SBK_STATUS_TRY_C_MSG(sc_..(..), "while starting '{}'", name);
SBK_STATUS_FAIL(SBK_ERR_BAKERY, "unreachable: {}", n);
return SBK_SUCCESS;
```

**Family 3 — your function returns `sbk_id`** (legacy lookups):

```cpp
SBK_ID_CHECK(std::filesystem::exists(file), SBK_ERR_INVALID_FILE);
SBK_ID_CHECK_MSG(id != 0, SBK_ERR_BAKERY_OBJECT_NOT_FOUND, "no object '{}'", name);
return foundId;
```

## Worked example: `system::create_project`

Internal functions return `sbk::result<void>`, so this uses the unprefixed family:

```cpp
auto system::create_project(const std::filesystem::directory_entry& dir,
                            std::string_view name) -> sbk::result<void>
{
    const sbk::editor::project_configuration cfg(dir, name);

    SBK_TRYV(open_project(cfg.project_file(), nullptr));                      // propagate a result<void> call
    SBK_CHECK_MSG(m_project != nullptr, SBK_ERR_BAKERY, "project was null");  // guard + context

    std::shared_ptr<bus> masterBus = m_project->create_database_object<bus>();
    SBK_CHECK(masterBus, SBK_ERR_OUT_OF_MEMORY);                             // generic guard

    masterBus->set_object_name("Master Bus");
    masterBus->set_master_bus(true);
    SBK_TRYV(m_project->save_project());
    return sbk::ok();
}
```

The whole internal engine works this way now. The `SBK_STATUS_` family survives only in the
public `sbk_*` C functions (`api/sound_bakery.cpp`), which return the raw code and bridge to
the rich implementation with `sbk::to_status`:

```cpp
sbk_result sbk_system_create()  // C ABI: must return sbk_result
{
    return sbk::to_status(sbk::engine::system::create());  // create() returns sbk::result<void>
}
```

## Bridging the two worlds

At the C ABI boundary, call the rich implementation and collapse it to a code:

```cpp
sbk_result sbk_system_update()                     // C ABI: must return sbk_result
{
    return sbk::to_status(system::get()->update());  // update() returns sbk::result<>
}
```

## Adding a new error code

Add it to the `sbk_result` enum in `inc/sound_chef/sound_chef_common.h`, then add a
matching `case` to `sbk::to_string()` in `error.cpp` so it logs with a readable name.
