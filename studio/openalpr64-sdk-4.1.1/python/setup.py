import setuptools

with open('README.md', 'r') as f:
    long_description = f.read()

setuptools.setup(
    name='openalpr',
    version='1.0.12',
    description='OpenALPR Python Bindings',
    long_description=long_description,	
    long_description_content_type='text/markdown',
    author='Matt Hill',
    author_email='matthill@openalpr.com',
    url='http://www.openalpr.com/',
    packages=['openalpr', 'vehicleclassifier']
)
