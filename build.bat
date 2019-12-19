
@echo Start build


@echo "$$$$$$$$$$$$$$$$$$$ start build backend $$$$$$$$$$$$$$$$$$$$$$$$$$"
MSBuild DSCADA.sln /t:Build
cd ..


