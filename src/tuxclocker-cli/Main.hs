{-# LANGUAGE OverloadedStrings #-}

import Data.Either
import Data.Functor
import Data.Maybe
import Data.Text (Text)
import Data.Tree
import DBus
import DBus.Client
import Text.Printf

import qualified DBus.Internal.Types as I
import qualified DBus.Introspection as I

data DeviceInterface =
  DynamicReadable

data DeviceNode = DeviceNode {
  object :: I.Object,
  interface :: Maybe DeviceInterface
}

data InterfaceValue = ValueReadableData ReadableData

data ReadableData = ReadableData {
  value :: ReadableValue,
  unit :: Maybe String
}

newtype DynamicReadableNode = DynamicReadableNode ObjectPath

newtype ReadableValue = ReadableValue I.Atom

instance Show InterfaceValue where
  show (ValueReadableData x) = show x

instance Show ReadableData where
  show x = maybe valStr (\u -> valStr <> " " <> u) $ unit x where
    valStr = show $ value x

instance Show ReadableValue where
  -- Show 2 decimals for Double
  show (ReadableValue (I.AtomDouble d)) = printf "%.2f" d
  show (ReadableValue x) = I.showAtom False x

maybeRight (Right x) = Just x
maybeRight _ = Nothing

tuxClockerCall :: MethodCall -> MethodCall
tuxClockerCall call = call { methodCallDestination = Just "org.tuxclocker" }

getObject :: Client -> ObjectPath -> IO I.Object
getObject client path = do
  reply <- call_ client $
    tuxClockerCall $ methodCall path "org.freedesktop.DBus.Introspectable" "Introspect"
  let xml = fromVariant (head $ methodReturnBody reply)
  pure $ fromMaybe
    (error ("Invalid introspection XML: " ++ show xml))
    (xml >>= I.parseXML path)

deviceInterface :: I.Object -> Maybe DeviceInterface
deviceInterface object =
  go where
    interfaceNames = I.interfaceName <$> I.objectInterfaces object
    hasInterface :: I.InterfaceName -> Bool
    hasInterface str = elem str interfaceNames
    go
      | hasInterface "org.tuxclocker.DynamicReadable" = Just DynamicReadable
      | otherwise = Nothing

getShowNode :: Client -> DeviceNode -> IO String
getShowNode client node = do
  name <- getName client $ I.objectPath $ object node
  ifaceValue <- getInterfaceValue client node
  pure $ name <> "\n" <> showMaybe ifaceValue where
    showMaybe (Just x) = show x
    showMaybe _ = ""

getInterfaceValue :: Client -> DeviceNode -> IO (Maybe InterfaceValue)
getInterfaceValue client node = go client node where
  go :: Client -> DeviceNode -> IO (Maybe InterfaceValue)
  go client node@DeviceNode { interface = Just DynamicReadable } = do
    data' <- getReadableData client (DynamicReadableNode $ I.objectPath $ object node)
    pure $ ValueReadableData <$> data'
  go _ _ = pure Nothing

getName :: Client -> ObjectPath -> IO String
getName client path =
  let
    call = tuxClockerCall $ methodCall path "org.tuxclocker.Node" "name"
  in
    getProperty client call <&> fromRight (toVariant ("Unnamed" :: String)) <&> variantToString

getValue :: Client -> DynamicReadableNode -> IO (Maybe ReadableValue)
getValue client (DynamicReadableNode path) =
  let
    _call = tuxClockerCall $ methodCall path "org.tuxclocker.DynamicReadable" "value"
    fromResult (False, I.Variant (I.ValueAtom value)) = Just $ ReadableValue value
    fromResult _ = Nothing
    toReadableValue x = fromVariant x >>= fromResult
  in do
    reply <- call_ client _call
    let variant = head $ methodReturnBody reply
    pure $ toReadableValue variant

-- TODO: no indication when fetching unit fails
getUnit :: Client -> DynamicReadableNode -> IO (Maybe String)
getUnit client (DynamicReadableNode path) =
  let
    call = tuxClockerCall $ methodCall path "org.tuxclocker.DynamicReadable" "unit"
    fromResult (False, unit) = Just unit
    fromResult _ = Nothing
  in do
    result <- getProperty client call
    pure $ maybeRight result >>= fromVariant >>= fromResult

getReadableData :: Client -> DynamicReadableNode -> IO (Maybe ReadableData)
getReadableData client node = do
  value <- getValue client node
  unit <- getUnit client node
  pure $ fmap (\value -> ReadableData value unit) value

getDBusTree :: Client -> IO (Tree I.Object)
getDBusTree client = unfoldTreeM (buildNode client) "/" where
  buildNode :: Client -> ObjectPath -> IO (I.Object, [ObjectPath])
  buildNode client path = do
    object <- getObject client path
    let childPaths = I.objectPath <$> I.objectChildren object
    pure (object, childPaths)

variantToString :: Variant -> String
variantToString x = fromMaybe "Invalid" $ fromVariant x

getShowTree :: Client -> Tree I.Object -> IO (Tree String)
getShowTree client =
  mapM (\obj -> getShowNode client $ DeviceNode obj $ deviceInterface obj)

main = do
  client <- connectSystem
  tree <- getDBusTree client
  treeShow <- getShowTree client tree
  putStr $ drawTree treeShow
