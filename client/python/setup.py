import setuptools

with open("README.md", "r") as fh:
    long_description = fh.read()

setuptools.setup(
    name="vdms",
    version="0.0.18",
    author="Chaunté W. Lacewell",
    author_email="chaunte.w.lacewell@intel.com",
    description="VDMS Client Module",
    install_requires=["protobuf==3.20.3"],
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://github.com/IntelLabs/vdms",
    license="MIT",
    packages=setuptools.find_packages(),
    python_requires=">=2.6, !=3.0.*, !=3.1.*, !=3.2.*, <4",
    classifiers=[
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: MIT License",
        "Operating System :: OS Independent",
    ],
)
