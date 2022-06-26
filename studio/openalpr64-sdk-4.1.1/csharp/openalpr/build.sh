#!/bin/bash

# First install .net Core APT packages

cp linux/alprnet.sln alprnet/
cp linux/AlprNet.csproj alprnet/
cp linux/AlprNetTest.csproj AlprNetTest/
rm AlprNetTest/Properties/AssemblyInfo.cs
rm alprnet/Properties/AssemblyInfo.cs

dotnet build alprnet/alprnet.sln
