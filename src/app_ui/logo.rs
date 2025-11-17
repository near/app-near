use ledger_device_sdk::include_gif;

#[cfg(any(target_os = "stax", target_os = "flex", target_os = "apex_p"))]
use ledger_device_sdk::nbgl::NbglGlyph;
#[cfg(any(target_os = "nanox", target_os = "nanosplus"))]
use ledger_device_sdk::ui::bitmaps::Glyph;

#[cfg(any(target_os = "stax", target_os = "flex"))]
pub const NEAR_LOGO: NbglGlyph =
    NbglGlyph::from_include(include_gif!("icons/app_near_64px.gif", NBGL));
#[cfg(target_os = "apex_p")]
pub const NEAR_LOGO: NbglGlyph =
    NbglGlyph::from_include(include_gif!("icons/app_near_48px.png", NBGL));
#[cfg(target_os = "nanosplus")]
pub const NEAR_LOGO: Glyph = Glyph::from_include(include_gif!("icons/app_near_14px.gif"));
#[cfg(target_os = "nanox")]
pub const NEAR_LOGO: Glyph = Glyph::from_include(include_gif!("icons/app_near_14px.gif"));
