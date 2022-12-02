# setup.sh
# (Only for reference, this should have been executed as an init script on the instance already)

wget https://repo.continuum.io/miniconda/Miniconda3-latest-Linux-x86_64.sh -O miniconda.sh
chmod +x miniconda.sh
./miniconda.sh -b
export PATH=$HOME/miniconda3/bin:$PATH
export PYTHONUNBUFFERED=1

conda install mpi4py=3.1.3 'mpich=4.0*=h*' mpi=1.0 'gcc_linux-64==11.2.0' 'gxx_linux-64==11.2.0' -c conda-forge --yes

echo "export PATH=$HOME/miniconda3/bin:$PATH" >> ~/.bashrc
echo "export PYTHONUNBUFFERED=1" >> ~/.bashrc
echo "source activate base" >> ~/.bashrc
echo "export LD_LIBRARY_PATH=$HOME/miniconda3/lib:$LD_LIBRARY_PATH" >> ~/.bashrc
echo "export PATH=$HOME/miniconda3/bin:$PATH" >> ~/.zshrc
echo "export PYTHONUNBUFFERED=1" >> ~/.zshrc
echo "source activate base" >> ~/.zshrc
echo "export LD_LIBRARY_PATH=$HOME/miniconda3/lib:$LD_LIBRARY_PATH" >> ~/.zshrc
