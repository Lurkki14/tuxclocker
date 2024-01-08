{-# LANGUAGE OverloadedStrings #-}

import Data.Either
import Data.Functor
import Data.Maybe
import Data.Text (Text)
import Data.Tree
import DBus
import DBus.Client
import qualified DBus.Internal.Types as I
import qualified DBus.Introspection as I

data DeviceInterface =
  DynamicReadable

data DeviceNode = DeviceNode {
  object :: I.Object,
  interface :: Maybe DeviceInterface
}

newtype DynamicReadableNode = DynamicReadableNode ObjectPath

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
  ifaceText <- getShowInterface client node
  pure $ name <> "\n" <> ifaceText

getShowInterface :: Client -> DeviceNode -> IO String
getShowInterface client node = go client node where
  go client node@DeviceNode { interface = Just DynamicReadable } =
    getValue client $ DynamicReadableNode $ I.objectPath $ object node
  go _ _ = pure ""

getName :: Client -> ObjectPath -> IO String
getName client path =
  let
    call = tuxClockerCall $ methodCall path "org.tuxclocker.Node" "name"
  in
    getProperty client call <&> fromRight (toVariant ("Unnamed" :: String)) <&> variantToString

getValue :: Client -> DynamicReadableNode -> IO String
getValue client (DynamicReadableNode path) =
  let
    _call = tuxClockerCall $ methodCall path "org.tuxclocker.DynamicReadable" "value"
  in do
    reply <- call_ client _call
    pure $ show $ head $ methodReturnBody reply

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
