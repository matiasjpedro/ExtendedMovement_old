
#include "EXMContractTypes.h"

#include "UObject/CoreNet.h"

bool FPredictionContractRequest::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	if (Ar.IsLoading())
	{
		FNetBitReader& NetReader = static_cast<FNetBitReader&>(Ar);

		bool bIsModifiersInInstigator = NetReader.ReadBit() == 1;

		if (bIsModifiersInInstigator)
		{
			NetReader << Instigator;
		}
		else
		{
			NetReader << InstigatorClass;

			bool bModifiersInContract = NetReader.ReadBit() == 1;

			if (bModifiersInContract)
				NetReader << SerializedData;
		}

		bool bHasDuration = NetReader.ReadBit() == 1;

		if (bHasDuration)
			NetReader << Duration;

		bool bHasModifierId = NetReader.ReadBit() == 1;

		if (bHasModifierId)
			NetReader << ModifierId;
	}
	else
	{
		FNetBitWriter& NetWriter = static_cast<FNetBitWriter&>(Ar);

		bool bIsModifiersInInstigator = IsModifierSources(EModifiersSources::ModifiersInInstigator);

		NetWriter.WriteBit(IsModifierSources(EModifiersSources::ModifiersInInstigator) ? 1 : 0);

		if (bIsModifiersInInstigator)
		{
			NetWriter << Instigator;
		}
		else
		{
			NetWriter << InstigatorClass;

			bool bModifiersInContract = SerializedData.Num() > 0;

			NetWriter.WriteBit(bModifiersInContract ? 1 : 0);

			if (bModifiersInContract)
				NetWriter << SerializedData;
		}

		bool bHasDuration = Duration != InvalidDuration;

		NetWriter.WriteBit(bHasDuration ? 1 : 0);

		if (bHasDuration)
			NetWriter << Duration;

		bool bHasModifierId = ModifierId != InvalidModifierID;

		NetWriter.WriteBit(bHasModifierId ? 1 : 0);

		if (bHasModifierId)
			NetWriter << ModifierId;
	}

	return true;
}
