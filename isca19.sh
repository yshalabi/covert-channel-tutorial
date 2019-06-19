echo "ISCA 2019 -- Side-Channel Tutorial -- Hands On Session!"
BINS_DIR=/isca19/bins/channels
mkdir -p ${BINS_DIR}/channels/Prime+Probe_L1D
mkdir -p ${BINS_DIR}/channels/Prime+Probe_LLC
mkdir -p ${BINS_DIR}/channels/Flush+Reload
mkdir -p ${BINS_DIR}/tools/contention
cp build/pp_l1d_send ${BINS_DIR}/Prime+Probe_L1D/send
cp build/pp_l1d_recv ${BINS_DIR}/Prime+Probe_L1D/recv
cp build/pp_llc_send ${BINS_DIR}/Prime+Probe_LLC/send
cp build/pp_llc_recv ${BINS_DIR}/Prime+Probe_LLC/recv
cp build/fr_send ${BINS_DIR}/Flush+Reload/send
cp build/fr_recv ${BINS_DIR}/Flush+Reload/recv
chown -R 1000:1000 ${BINS_DIR}
