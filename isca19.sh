echo "ISCA 2019 -- Side-Channel Tutorial -- Hands On Session!"
BINS_DIR=/isca19/bins
mkdir -p ${BINS_DIR}/send
mkdir -p ${BINS_DIR}/recv
cp build/*send* ${BINS_DIR}/send
cp build/*recv* ${BINS_DIR}/recv
