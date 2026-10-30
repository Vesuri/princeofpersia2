#pragma once
// No OS calls in global constructors: SysBase must already be initialized.
class PlatformAmiga { public: bool run(); const char* reason() const; };
