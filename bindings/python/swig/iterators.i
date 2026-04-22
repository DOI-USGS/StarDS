using namespace star;

%extend star::NDArray {
    %pythoncode %{
    def __iter__(self):
        """Iterate over flat array elements"""
        for i in range(self.size()):
            yield self.flat(i)

    def __getitem__(self, key):
        """Support arr[i] for flat indexing"""
        if isinstance(key, int):
            if key < 0:
                key += self.size()
            if key < 0 or key >= self.size():
                raise IndexError("Index out of range")
            return self.flat(key)
        else:
            raise TypeError("NDArray indices must be integers")
    %}
}
