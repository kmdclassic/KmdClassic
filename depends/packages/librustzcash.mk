package=librustzcash
$(package)_version=0.2.0
$(package)_download_path=https://github.com/zcash/$(package)/archive/
$(package)_file_name=$(package)-$($(package)_git_commit).tar.gz
$(package)_download_file=$($(package)_git_commit).tar.gz
$(package)_sha256_hash=dfb80e9a57d944a91092094a423a8a6631e38b602b337aad5f98dc21002ca6dc
$(package)_git_commit=a57dc7f47807ea50cb0a5deec9b84b3e7da11bc0
$(package)_dependencies=rust $(rust_crates)
$(package)_patches=cargo.config remove-dev-dependencies.diff

$(package)_rust_target=$(if $(rust_rust_target_$(canonical_host)),$(rust_rust_target_$(canonical_host)),$(canonical_host))
$(package)_library_file=target/release/librustzcash.a

define $(package)_set_vars
$(package)_build_opts=--frozen --release
$(package)_build_opts_mingw32=--target=x86_64-pc-windows-gnu
$(package)_build_opts_aarch64_linux=--target=aarch64-unknown-linux-gnu
$(package)_build_opts_x86_64_darwin=--target=x86_64-apple-darwin
$(package)_build_opts_aarch64_darwin=--target=aarch64-apple-darwin
endef

define $(package)_preprocess_cmds
  patch -p1 < $($(package)_patch_dir)/remove-dev-dependencies.diff && \
  mkdir .cargo && \
  cat $($(package)_patch_dir)/cargo.config | sed 's|CRATE_REGISTRY|$(host_prefix)/$(CRATE_REGISTRY)|' > .cargo/config
endef

# $(host_prefix)/native/bin/cargo build --package librustzcash $($(package)_build_opts)
define $(package)_build_cmds
  RUSTFLAGS="${RUSTFLAGS} -A unused_mut" cargo build --package librustzcash $($(package)_build_opts)
endef

define $(package)_stage_cmds
  [ -f target/$($(package)_rust_target)/release/librustzcash.a ] && \
    cp target/$($(package)_rust_target)/release/librustzcash.a $($(package)_library_file) || true && \
  mkdir -p $($(package)_staging_dir)$(host_prefix)/lib/ && \
  mkdir -p $($(package)_staging_dir)$(host_prefix)/include/ && \
  cp $($(package)_library_file) $($(package)_staging_dir)$(host_prefix)/lib/ && \
  cp librustzcash/include/librustzcash.h $($(package)_staging_dir)$(host_prefix)/include/
endef
