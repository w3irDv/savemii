FROM ghcr.io/wiiu-env/devkitppc:20240704

COPY --from=ghcr.io/wiiu-env/libmocha:20240603 /artifacts $DEVKITPRO

RUN git clone --recursive https://github.com/yawut/libromfs-wiiu --single-branch && \
  cd libromfs-wiiu && \
  make -j$(nproc) && \
  make install && \
  cd .. && \
  rm -rf libromfs-wiiu

WORKDIR /project
